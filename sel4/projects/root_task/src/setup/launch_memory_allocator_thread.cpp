#include "setup.h"

#include "globals/globals.h"
#include <memory_allocator/memory_allocator.h>

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
static seL4_Word current_free_paging_cap = GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_paging_objects_start);

static bool createVSpace() {
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_X64_PML4Object, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_vspace), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator vspace");
    
    error = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace));
    retErrorIfFail(error == seL4_NoError, "Failed to assign to ASID pool during thread setup");
    
    return true;
}

static bool createCSpace() {
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_CapTableObject, GLOBALS_SMALL_CNODE_BITS,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_croot), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator cspace");

    return true;
}

static bool readElfHeader() {
    ElfParser_Error ep_err = elfparser_get_header(memory_allocator_elf_start, memory_allocator_elf_size, &elf_header);
    retErrorIfFail(ep_err == ELFPARSER_NOERROR, "Error reading memory allocator elf header");
    return true;
}

static bool setupMemoryAllocatorFrames() {
    // Create CNode to hold frame caps
    
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_CapTableObject, GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, GLOBALS_CSLOT_INDEX(temp_slot), 1);
    retErrorIfFail(error == seL4_NoError, "Failed to create memory allocator frames cnode");
    
    error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), GLOBALS_SMALL_CNODE_BITS,
                              seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), GLOBALS_SMALL_CNODE_BITS,
                              seL4_WordBits - GLOBALS_SMALL_CNODE_BITS - GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS); // Set guard so frames are accessed at depth of seL4_WordBits
    retErrorIfFail(error == seL4_NoError, "Failed to mutate memory allocator frames cnode");

    // Break chunk into 4k frames
    error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(memory_allocator_chunk), seL4_X86_4K, 0,
                                seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), GLOBALS_SMALL_CNODE_BITS, 0, BIT(GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS));
    retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator frames");

    return true;
}

static bool mapNewPagingStructure(seL4_Word type, seL4_CPtr vspace, seL4_Word address) {
    retErrorIfFail(current_free_paging_cap <= GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_paging_objects_end), "Ran out of paging caps while loading memory allocator");
    
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), type, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS, current_free_paging_cap, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into x86 page directory object");
    
    switch(type) {
        case seL4_X86_PageTableObject:
            error = seL4_X86_PageTable_Map(GLOBALS_CSLOT(assorted_caps) | current_free_paging_cap, vspace, address, seL4_X86_Default_VMAttributes);
            break;
        case seL4_X86_PageDirectoryObject:
            error = seL4_X86_PageDirectory_Map(GLOBALS_CSLOT(assorted_caps) | current_free_paging_cap, vspace, address, seL4_X86_Default_VMAttributes);
            break;
        case seL4_X86_PDPTObject:
            error = seL4_X86_PDPT_Map(GLOBALS_CSLOT(assorted_caps) | current_free_paging_cap, vspace, address, seL4_X86_Default_VMAttributes);
            break;
    }
    current_free_paging_cap++;
    retErrorIfFail(error == seL4_NoError, "Failed to create memory allocator paging structure");
    return true;
}

static bool mapCurrentFrame(seL4_CPtr vspace, seL4_Word addr_to_map_at) {
    // Make sure we have a free frame
    retErrorIfFail(current_frame < BIT(GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS), "Ran out of frames while loading memory allocator");
    
    seL4_Error error = seL4_X86_Page_Map(GLOBALS_CSLOT(memory_allocator_frames) | current_frame, vspace, addr_to_map_at,
                                         seL4_ReadWrite, seL4_X86_Default_VMAttributes);
    if (error == seL4_NoError) {
        current_frame++;
        return true;
    }
	retErrorIfFail(error == seL4_FailedLookup, "Error mapping memory allocator frame"); // Failed lookup is ok, we just need to map more structures
    
    seL4_Word failed_level = seL4_MappingFailedLookupLevel();
    
    // Need to map PDPT
    if (failed_level >= 39) {
        retFalseIfFail(mapNewPagingStructure(seL4_X86_PDPTObject, vspace, addr_to_map_at));
    }
    
    // Need to map PD
    if (failed_level >= 30) {
        retFalseIfFail(mapNewPagingStructure(seL4_X86_PageDirectoryObject, vspace, addr_to_map_at));
    }
    
    // Need to map PT
    if (failed_level >= 21) {
        retFalseIfFail(mapNewPagingStructure(seL4_X86_PageTableObject, vspace, addr_to_map_at));
    }
    
    // Retry mapping
    error = seL4_X86_Page_Map(GLOBALS_CSLOT(memory_allocator_frames) | current_frame, vspace, addr_to_map_at,
                              seL4_ReadWrite, seL4_X86_Default_VMAttributes);
	retErrorIfFail(error == seL4_NoError, "Error mapping memory allocator frame");
    current_frame++;
    return true;
}

static bool unmapCurrentFrame() {
    current_frame--;
    seL4_Error error = seL4_X86_Page_Unmap(GLOBALS_CSLOT(memory_allocator_frames) | current_frame);
    retErrorIfFail(error == seL4_NoError, "Error unmapping memory allocator frame");
    return true;
}

static bool loadProgramHeader(ElfParser_ProgramHeader ph) {
    uint64_t total_bytes = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index, NULL, 0, 0);
    retErrorIfFail(total_bytes != ELFPARSER_INVALID, "Error loading memory allocator elf program header");
    
    uint64_t offset = ph.p_vaddr % BIT(GLOBALS_SMALL_CHUNK_BITS);
    uint64_t bytes_left = total_bytes;
    seL4_Word dest_vaddr = ph.p_vaddr - offset;
    while (bytes_left > 0) {
        // First map frame into our address space to do the copying
        retFalseIfFail(mapCurrentFrame(seL4_CapInitThreadVSpace, TEMP_FRAME_VADDR));
        
        bytes_left = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index,
                                            (void*)(TEMP_FRAME_VADDR + offset), total_bytes - bytes_left, BIT(GLOBALS_SMALL_CHUNK_BITS) - offset);
        retErrorIfFail(bytes_left != ELFPARSER_INVALID, "Error loading memory allocator elf program header");
        
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
    
    for (uint64_t i = 0; i < elf_header.e_phnum; i++) {
        ElfParser_ProgramHeader program_header;
        ElfParser_Error ep_err = elfparser_get_program_header(memory_allocator_elf_start, &elf_header, i, &program_header);
        retErrorIfFail(ep_err == ELFPARSER_NOERROR, "Error reading memory allocator elf program header");

        if (program_header.p_type != ELFPARSER_PT_LOAD) continue;
        retErrorIfFail(program_header.p_align >= BIT(GLOBALS_SMALL_CHUNK_BITS), "Memory allocator elf program header has improper alignment");
        
        retFalseIfFail(loadProgramHeader(program_header));
    }
    return true;
}

static bool setupStack() {
    // The top of the stack is at the end of the page, so we actually need to map the page under the stack top address
    retErrorIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), THREAD_STACK_TOP_VADDR - BIT(GLOBALS_SMALL_CHUNK_BITS)), "Failed to map memory allocator stack");
    return true;
}

static bool createIPCBuffer() {
    seL4_Word ipc_frame = current_frame;
    
    retErrorIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), THREAD_IPC_BUFFER_VADDR), "Failed to map memory allocator IPC buffer");
    
    seL4_Error error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer), seL4_WordBits,
                                       seL4_CapInitThreadCNode, GLOBALS_CSLOT(memory_allocator_frames) | ipc_frame, seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to copy memory allocator IPC buffer frame");
    
    return true;
}

static bool setupTCB() {
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_TCBObject, seL4_TCBBits,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_tcb), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into memory allocator TCB");

    error = seL4_TCB_Configure(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapNull,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), seL4_WordBits - GLOBALS_SMALL_CNODE_BITS,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), 0,
                               THREAD_IPC_BUFFER_VADDR, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer));
    retErrorIfFail(error == seL4_NoError, "Failed to configure memory allocator TCB");
    
    error = seL4_TCB_SetPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	retErrorIfFail(error == seL4_NoError, "Failed to set priority during thread setup");
    
    error = seL4_TCB_SetMCPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	retErrorIfFail(error == seL4_NoError, "Failed to set maximum controlled priority during thread setup");
    
    seL4_DebugNameThread(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), "Memory Allocator");

    return true;
}

static bool setupRegisters() {
    seL4_UserContext regs;
    seL4_Error error = seL4_TCB_ReadRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to read memory allocator registers");

	regs.rip = (seL4_Word)elf_header.e_entry;
	regs.rsp = THREAD_STACK_TOP_VADDR; // Set stack pointer to top of stack
	regs.rdi = 42;

	error = seL4_TCB_WriteRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to write memory allocator registers");
    
    return true;
}

static bool setupTLS() {
    retErrorIfFail(mapCurrentFrame(seL4_CapInitThreadVSpace, TEMP_FRAME_VADDR), "Failed to map memory allocator TLS in root task address space");

    seL4_Word tls = sel4runtime_write_tls_image((void*)(TEMP_FRAME_VADDR));

	int error = sel4runtime_set_tls_variable(tls, __sel4_ipc_buffer, (seL4_IPCBuffer*)THREAD_IPC_BUFFER_VADDR);
	retErrorIfFail(error == seL4_NoError, "Failed to set memory allocator ipc_buffer TLS variable");
    
    // Adjust base to destination address
	error = seL4_TCB_SetTLSBase(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), tls + THREAD_TLS_VADDR - BOOTSTRAP_VADDR);
	retErrorIfFail(error == seL4_NoError, "Failed to set memory allocator TLS base");
    
    retFalseIfFail(unmapCurrentFrame());
    retErrorIfFail(mapCurrentFrame(GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), THREAD_TLS_VADDR), "Failed to map memory allocator TLS");
    return true;
}

static bool setupCSlots() {
    seL4_Error error = seL4_CNode_Copy(GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), 0, GLOBALS_SMALL_CNODE_BITS,
                                       seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_WordBits, seL4_AllRights);
    retErrorIfFail(error == seL4_NoError, "Failed to move memory allocator TCB while setting up CSlots");
    
    return true;
}

static bool setupMemoryAllocatorThread() {
    retFalseIfFail(createVSpace());
    retFalseIfFail(createCSpace());
    retFalseIfFail(loadElf());
    retFalseIfFail(setupStack());
    retFalseIfFail(createIPCBuffer());
    retFalseIfFail(setupTCB());
    retFalseIfFail(setupTLS());
    retFalseIfFail(setupRegisters());
    retFalseIfFail(setupCSlots());
    
    return true;
}

static bool resumeMemoryAllocatorThread() {
    seL4_Error error = seL4_TCB_Resume(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb));
    retErrorIfFail(error == seL4_NoError, "Failed to resume memory allocator thread");
    return true;
}

bool Setup::launchMemoryAllocatorThread() {
    retFalseIfFail(setupMemoryAllocatorThread());
    retFalseIfFail(resumeMemoryAllocatorThread());
    return true;
}