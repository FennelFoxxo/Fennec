#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"
#include "mapping.hpp"
#include <memory_allocator/memory_allocator.h>

#include <stack.hpp>

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4/sel4_arch/mapping.h>
#include <sel4runtime.h>
#include <utils/util.h>
#include <elfparser.h>
}

static ElfParser_Header elf_header;
static seL4_Word current_frame = 0;
static seL4_Word current_free_paging_cap = GLOBALS_ASSORTED_CSLOT(memory_allocator_paging_objects_start);
static seL4_Word stack_bytes_pushed = 0;
static seL4_CPtr untyped_cptr, return_cptr;


static bool getMappingCSlotFunc(seL4_CPtr* cptr) {
    retErrorIfFail(current_free_paging_cap != GLOBALS_ASSORTED_CSLOT(memory_allocator_paging_objects_end), "Ran out of mapping caps!");
    *cptr = current_free_paging_cap++;
    return true;
}


static MappingContext dest_vspace_mapping_context(
    GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), MemTree::getFreeUntyped, MemTree::returnUsedUntyped,
    getMappingCSlotFunc, {0, 0, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS}
);

static bool getFrame() {
    return MemTree::getFreeUntyped(&untyped_cptr, &return_cptr);
}

static bool returnFrame() {
    return MemTree::returnUsedUntyped(return_cptr);
}

static bool createVSpace() {
    retFalseIfFail(getFrame());
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_X64_PML4Object, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_vspace), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator vspace!");
    
    retFalseIfFail(returnFrame());
    
    error = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace));
    retErrorIfFail(error == seL4_NoError, "Failed to assign to ASID pool during thread setup!");
    
    return true;
}

static bool createCSpace() {
    retFalseIfFail(getFrame());
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, GLOBALS_SMALL_CNODE_BITS,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_croot), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator cspace!");
    
    retFalseIfFail(returnFrame());

    return true;
}

static bool readElfHeader() {
    ElfParser_Error ep_err = elfparser_get_header(memory_allocator_elf_start, memory_allocator_elf_size, &elf_header);
    retErrorIfFail(ep_err == ELFPARSER_NOERROR, "Error reading memory allocator elf header!");
    return true;
}

static bool setupMemoryAllocatorFrames() {
    // Create CNode to hold frame caps
    retFalseIfFail(getFrame());
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, GLOBALS_CSLOT_INDEX(temp_slot), 1);
    retErrorIfFail(error == seL4_NoError, "Failed to create memory allocator frames cnode");
    
    retFalseIfFail(returnFrame());
    
    error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), PAGE_CNODE_BITS,
                              seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS,
                              seL4_WordBits - 2 * PAGE_CNODE_BITS); // Set guard so frames are accessed at depth of seL4_WordBits
    retErrorIfFail(error == seL4_NoError, "Failed to mutate memory allocator frames cnode");

    
    // Break chunk into 4k frames
    for (seL4_Word i = 0; i < BIT(PAGE_CNODE_BITS); i++) {
        retFalseIfFail(getFrame());
        
        error = seL4_Untyped_Retype(untyped_cptr, seL4_X86_4K, 0,
                                seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), PAGE_CNODE_BITS, i, 1);
        retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator frames");
        
        retFalseIfFail(returnFrame());
    }

    return true;
}

static bool mapCurrentFrame(seL4_CPtr vspace, seL4_Word addr_to_map_at) {
    // Make sure we have a free frame
    retErrorIfFail(current_frame < BIT(GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS), "Ran out of frames while loading memory allocator!");
    
    if (vspace == seL4_CapInitThreadVSpace) {
        return Globals::mapping_context.mapFrame(GLOBALS_CSLOT(memory_allocator_frames) | current_frame++, addr_to_map_at);
    }
    
    return dest_vspace_mapping_context.mapFrame(GLOBALS_CSLOT(memory_allocator_frames) | current_frame++, addr_to_map_at);
}

static bool unmapCurrentFrame() {
    current_frame--;

    return Globals::mapping_context.unmapFrame(GLOBALS_CSLOT(memory_allocator_frames) | current_frame);

}

static bool loadProgramHeader(ElfParser_ProgramHeader ph) {
    uint64_t total_bytes = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index, NULL, 0, 0);
    retErrorIfFail(total_bytes != ELFPARSER_INVALID, "Error loading memory allocator elf program header!");
    
    uint64_t offset = ph.p_vaddr % BIT(GLOBALS_SMALL_CHUNK_BITS);
    uint64_t bytes_left = total_bytes;
    seL4_Word dest_vaddr = ph.p_vaddr - offset;
    while (bytes_left > 0) {
        // First map frame into our address space to do the copying
        retFalseIfFail(mapCurrentFrame(seL4_CapInitThreadVSpace, TEMP_FRAME_VADDR));
        
        bytes_left = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index,
                                            (void*)(TEMP_FRAME_VADDR + offset), total_bytes - bytes_left, BIT(GLOBALS_SMALL_CHUNK_BITS) - offset);
        retErrorIfFail(bytes_left != ELFPARSER_INVALID, "Error loading memory allocator elf program header!");
        
        // After offset into initial frame is accounted for, we don't need to worry about it anymore
        offset = 0;
        
        // Remap frame into thread address space
        retFalseIfFail(unmapCurrentFrame());
        retFalseIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), dest_vaddr));
        printf("Mapped at %lx\n", dest_vaddr);
        dest_vaddr += BIT(GLOBALS_SMALL_CHUNK_BITS);
        
    }

    return true;
}

static bool loadElf() {
    retFalseIfFail(setupMemoryAllocatorFrames());
    retFalseIfFail(readElfHeader());
    
    uint64_t total_size = 0;
    
    for (uint64_t i = 0; i < elf_header.e_phnum; i++) {
        ElfParser_ProgramHeader program_header;
        ElfParser_Error ep_err = elfparser_get_program_header(memory_allocator_elf_start, &elf_header, i, &program_header);
        retErrorIfFail(ep_err == ELFPARSER_NOERROR, "Error reading memory allocator elf program header!");

        if (program_header.p_type != ELFPARSER_PT_LOAD) continue;
        retErrorIfFail(program_header.p_align >= BIT(GLOBALS_SMALL_CHUNK_BITS), "Memory allocator elf program header has improper alignment!");
        
        total_size += program_header.p_memsz;
        retFalseIfFail(loadProgramHeader(program_header));
    }
    printf("Total size: %lu\n", total_size);
    return true;
}

static bool setupStack() {
    retFalseIfFail(mapCurrentFrame(seL4_CapInitThreadVSpace, TEMP_FRAME_VADDR));
    
    // Create stack object at top of page
    long long unsigned temp_stack_top_initial = TEMP_FRAME_VADDR + BIT(GLOBALS_SMALL_CHUNK_BITS);
    Stack stack(temp_stack_top_initial);
    
    const char* process_name = (const char*)stack.pushString("My process!");
    
    
    // Align stack
    while ((temp_stack_top_initial - stack.getStackTop()) % sizeof(seL4_Word)) {
        stack.push<char>(0);
    }

    // Auxiliary vector - null terminator
    stack.push(auxv_t{.a_type = AT_NULL});
    
    // Provide IPC buffer address
    stack.push(auxv_t{.a_type = AT_SEL4_IPC_BUFFER_PTR, .a_un{.a_ptr = (void*)THREAD_IPC_BUFFER_VADDR} });
    
    
    // Environment pointer vector - null terminator
    stack.push(SEL4RUNTIME_NULL);
    
    // Empty
    stack.push<seL4_Word>(0);

    // Second argument
    stack.push<seL4_Word>(22);

    // Push process name as first argument
    stack.push(process_name - temp_stack_top_initial + THREAD_STACK_TOP_VADDR);
    
    // Argument count
    stack.push<seL4_Word>(2);
    
    stack_bytes_pushed = temp_stack_top_initial - stack.getStackTop();
    
    retFalseIfFail(unmapCurrentFrame());
    
    // The top of the stack is at the end of the page, so we actually need to map the page under the stack top address
    retErrorIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), THREAD_STACK_TOP_VADDR - BIT(GLOBALS_SMALL_CHUNK_BITS)), "Failed to map memory allocator stack!");

    return true;
}

static bool createIPCBuffer() {
    seL4_Word ipc_frame = current_frame;
    
    retErrorIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), THREAD_IPC_BUFFER_VADDR), "Failed to map memory allocator IPC buffer!");
    
    seL4_Error error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer), seL4_WordBits,
                                       seL4_CapInitThreadCNode, GLOBALS_CSLOT(memory_allocator_frames) | ipc_frame, seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to copy memory allocator IPC buffer frame!");
    
    return true;
}

static bool setupTCB() {
    retFalseIfFail(getFrame());
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_TCBObject, seL4_TCBBits,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_tcb), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator TCB!");
    
    retFalseIfFail(returnFrame());

    error = seL4_TCB_Configure(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapNull,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), seL4_WordBits - GLOBALS_SMALL_CNODE_BITS,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), 0,
                               THREAD_IPC_BUFFER_VADDR, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer));
    retErrorIfFail(error == seL4_NoError, "Failed to configure memory allocator TCB!");
    
    error = seL4_TCB_SetPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	retErrorIfFail(error == seL4_NoError, "Failed to set priority during thread setup!");
    
    error = seL4_TCB_SetMCPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	retErrorIfFail(error == seL4_NoError, "Failed to set maximum controlled priority during thread setup!");
    
    seL4_DebugNameThread(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), "Memory Allocator");

    return true;
}

static bool setupRegisters() {
    seL4_UserContext regs;
    seL4_Error error = seL4_TCB_ReadRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to read memory allocator registers!");

	regs.rip = (seL4_Word)elf_header.e_entry;
	regs.rsp = THREAD_STACK_TOP_VADDR - stack_bytes_pushed; // Set stack pointer to top of stack

	error = seL4_TCB_WriteRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to write memory allocator registers!");
    
    return true;
}

static bool setupCSlots() {
    seL4_Error error = seL4_CNode_Copy(GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), 0, GLOBALS_SMALL_CNODE_BITS,
                                       seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_WordBits, seL4_AllRights);
    retErrorIfFail(error == seL4_NoError, "Failed to move memory allocator TCB while setting up CSlots!");
    
    return true;
}

static bool setupMemoryAllocatorThread() {
    retFalseIfFail(createVSpace());
    retFalseIfFail(createCSpace());
    retFalseIfFail(loadElf());
    retFalseIfFail(setupStack());
    retFalseIfFail(createIPCBuffer());
    retFalseIfFail(setupTCB());
    retFalseIfFail(setupRegisters());
    retFalseIfFail(setupCSlots());
    
    return true;
}

static bool resumeMemoryAllocatorThread() {
    seL4_Error error = seL4_TCB_Resume(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb));
    retErrorIfFail(error == seL4_NoError, "Failed to resume memory allocator thread!");
    return true;
}

bool Setup::launchMemoryAllocatorThread() {
    retFalseIfFail(setupMemoryAllocatorThread());
    seL4_DebugDumpScheduler();
    retFalseIfFail(resumeMemoryAllocatorThread());
    return true;
}