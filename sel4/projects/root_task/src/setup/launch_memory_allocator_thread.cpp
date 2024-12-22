#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"
#include "mapping.hpp"
#include <memory_allocator/memory_allocator.h>

#include <runtime_env.hpp>

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4/sel4_arch/mapping.h>
#include <sel4runtime.h>
#include <utils/util.h>
#include <elfparser.h>
}

static ElfParser_Header elf_header;

// Number of untypeds that have been retyped into page-sized frames.
static seL4_Word num_frames = 0;

// Number of frames that have been mapped into the vspace
// Also serves as the index of the next unmapped frame
static seL4_Word num_mapped_frames = 0;

// Keep track of free cslots for paging objects (page table, page directory, etc)
static seL4_Word next_free_paging_cslot = GLOBALS_ASSORTED_CSLOT(memory_allocator_paging_objects_start);

// Keep track of how many bytes we've pushed into the thread's stack
static seL4_Word stack_bytes_pushed = 0;

// Holds cslots returned by getUntyped()
static seL4_CPtr untyped_cptr, return_cptr;


static void getMappingCSlotFunc(seL4_CPtr* cptr) {
    assert(next_free_paging_cslot != GLOBALS_ASSORTED_CSLOT(memory_allocator_paging_objects_end));
    *cptr = next_free_paging_cslot++;
}

static MappingContext dest_vspace_mapping_context(
    GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), MemTree::getFreeUntyped, MemTree::returnUsedUntyped,
    getMappingCSlotFunc, {0, 0, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS}
);


static void getUntyped() {
    MemTree::getFreeUntyped(&untyped_cptr, &return_cptr);
}

static void returnUntyped() {
    MemTree::returnUsedUntyped(return_cptr);
}

static void retypeNewFrameIfNeeded() {
    // No need to retype a new frame if we have enough
    if (num_frames > num_mapped_frames) return;
    
    // Make sure we have space for a frame
    assert(num_frames < BIT(PAGE_CNODE_BITS));
    
    getUntyped(); // Get an untyped to use
    
    // Retype into page frame
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_X86_4K, 0,
                                            seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), PAGE_CNODE_BITS, num_frames, 1);
    assert(error == seL4_NoError, error);
    
    returnUntyped(); // Return the untyped
    
    num_frames++;
}

static void mapCurrentFrameInThisVSpace(seL4_Word vaddr) {
    retypeNewFrameIfNeeded();
    assert(Globals::mapping_context.mapFrame(GLOBALS_CSLOT(memory_allocator_frames) | num_mapped_frames++, vaddr));
}

static void mapCurrentFrameInDestVSpace(seL4_Word vaddr) {
    retypeNewFrameIfNeeded();
    assert(dest_vspace_mapping_context.mapFrame(GLOBALS_CSLOT(memory_allocator_frames) | num_mapped_frames++, vaddr));
}

static void unmapCurrentFrame() {
    num_mapped_frames--;
    assert(Globals::mapping_context.unmapFrame(GLOBALS_CSLOT(memory_allocator_frames) | num_mapped_frames));
}



static void createVSpace() {
    getUntyped();
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_X64_PML4Object, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_vspace), 1);
	assert(error == seL4_NoError, error);
    
    returnUntyped();
    
    error = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace));
    assert(error == seL4_NoError, error);
}

static void createCSpace() {
    getUntyped();
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_croot), 1);
	assert(error == seL4_NoError, error);
    
    returnUntyped();
}

static void readElfHeader() {
    ElfParser_Error ep_err = elfparser_get_header(memory_allocator_elf_start, memory_allocator_elf_size, &elf_header);
    assert(ep_err == ELFPARSER_NOERROR);
}

static void setupMemoryAllocatorFrames() {
    // Create CNode to hold frame caps
    getUntyped();
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, GLOBALS_CSLOT_INDEX(temp_slot), 1);
    assert(error == seL4_NoError, error);
    
    returnUntyped();
    
    error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(memory_allocator_frames), PAGE_CNODE_BITS,
                              seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS,
                              seL4_WordBits - 2 * PAGE_CNODE_BITS); // Set guard so frames are accessed at depth of seL4_WordBits
    assert(error == seL4_NoError, error);
}

static void loadProgramHeader(ElfParser_ProgramHeader ph) {
    uint64_t total_bytes = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index, NULL, 0, 0);
    assert(total_bytes != ELFPARSER_INVALID);
    
    uint64_t offset = ph.p_vaddr % BIT(seL4_PageBits);
    uint64_t bytes_left = total_bytes;
    seL4_Word dest_vaddr = ph.p_vaddr - offset;
    while (bytes_left > 0) {
        // First map frame into our address space to do the copying
        mapCurrentFrameInThisVSpace(TEMP_FRAME_VADDR);
        
        bytes_left = elfparser_copy_segment(memory_allocator_elf_start, &elf_header, ph.index,
                                            (void*)(TEMP_FRAME_VADDR + offset), total_bytes - bytes_left, BIT(seL4_PageBits) - offset);
        assert(bytes_left != ELFPARSER_INVALID);
        
        // After offset into initial frame is accounted for, we don't need to worry about it anymore
        offset = 0;
        
        // Remap frame into thread address space
        unmapCurrentFrame();
        mapCurrentFrameInDestVSpace(dest_vaddr);

        dest_vaddr += BIT(seL4_PageBits);
    }
}

static void loadElf() {
    setupMemoryAllocatorFrames();
    readElfHeader();
    
    uint64_t total_size = 0;
    
    for (uint64_t i = 0; i < elf_header.e_phnum; i++) {
        ElfParser_ProgramHeader program_header;
        ElfParser_Error ep_err = elfparser_get_program_header(memory_allocator_elf_start, &elf_header, i, &program_header);
        assert(ep_err == ELFPARSER_NOERROR);

        if (program_header.p_type != ELFPARSER_PT_LOAD) continue;
        assert(program_header.p_align >= BIT(seL4_PageBits));
        
        total_size += program_header.p_memsz;
        loadProgramHeader(program_header);
    }
    printf("Total size: %lu\n", total_size);
}

static void setupStack() {
    mapCurrentFrameInThisVSpace(TEMP_FRAME_VADDR);

    RuntimeEnvironment re;
    
    re.setStackTop(THREAD_STACK_TOP_VADDR);
    re.setProcessName("My process!");
    re.setIPCBufferAddress(THREAD_IPC_BUFFER_VADDR);
    re.addArg(50);
    
    int arr[10] = {10, 20, 30, 40, 50};
    int arr2[10] = {1, 2, 3, 4, 5};
    
    seL4_Word arr_addr2 = re.addCustomData(arr2, sizeof(arr2));
    seL4_Word arr_addr = re.addCustomData(arr, sizeof(arr));
    re.addArg(arr_addr);
    re.addArg(arr_addr2);
    
    // Write stack at top of page
    stack_bytes_pushed = re.write(TEMP_FRAME_VADDR + BIT(seL4_PageBits));
    
    
    unmapCurrentFrame();
    
    // The top of the stack is at the end of the page, so we actually need to map the page under the stack top address
    mapCurrentFrameInDestVSpace(THREAD_STACK_TOP_VADDR - BIT(seL4_PageBits));
}

static void createIPCBuffer() {
    // We need to pass the cap to the IPC frame to the memory allocator thread, so we need to save the location of the frame we're about to map
    seL4_Word ipc_frame = num_mapped_frames;
    
    mapCurrentFrameInDestVSpace(THREAD_IPC_BUFFER_VADDR);
    
    seL4_Error error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer), seL4_WordBits,
                                       seL4_CapInitThreadCNode, GLOBALS_CSLOT(memory_allocator_frames) | ipc_frame, seL4_WordBits);
    assert(error == seL4_NoError, error);
}

static void setupTCB() {
    getUntyped();
    
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_TCBObject, seL4_TCBBits,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                           GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_tcb), 1);
	assert(error == seL4_NoError, error);
    
    returnUntyped();

    error = seL4_TCB_Configure(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapNull,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), seL4_WordBits - PAGE_CNODE_BITS,
                               GLOBALS_ASSORTED_CSLOT(memory_allocator_vspace), 0,
                               THREAD_IPC_BUFFER_VADDR, GLOBALS_ASSORTED_CSLOT(memory_allocator_ipc_buffer));
    assert(error == seL4_NoError, error);
    
    error = seL4_TCB_SetPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	assert(error == seL4_NoError, error);
    
    error = seL4_TCB_SetMCPriority(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_CapInitThreadTCB, MEMORY_ALLOCATOR_PRIORITY);
	assert(error == seL4_NoError, error);
    
    seL4_DebugNameThread(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), "Memory Allocator");
}

static void setupRegisters() {
    seL4_UserContext regs;
    seL4_Error error = seL4_TCB_ReadRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	assert(error == seL4_NoError, error);

	regs.rip = (seL4_Word)elf_header.e_entry;
	regs.rsp = THREAD_STACK_TOP_VADDR - stack_bytes_pushed; // Set stack pointer to top of stack

	error = seL4_TCB_WriteRegisters(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	assert(error == seL4_NoError, error);
}

static void setupCSlots() {
    // Copy TCB cap into slot 0
    seL4_Error error = seL4_CNode_Copy(GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), 0, PAGE_CNODE_BITS,
                                       seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb), seL4_WordBits, seL4_AllRights);
    assert(error == seL4_NoError, error);
    
    // Create endpoint object
    getUntyped();
    error = seL4_Untyped_Retype(untyped_cptr, seL4_EndpointObject, PAGE_CNODE_BITS,
                                seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                GLOBALS_ASSORTED_CSLOT_INDEX(memory_allocator_endpoint), 1);
	assert(error == seL4_NoError, error);
    
    returnUntyped();
    
    // Copy endpoint object into slot 1
    error = seL4_CNode_Mint(GLOBALS_ASSORTED_CSLOT(memory_allocator_croot), 1, PAGE_CNODE_BITS,
                            seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_endpoint), seL4_WordBits, seL4_AllRights, 0x4269);
    assert(error == seL4_NoError, error);
}

static void setupMemoryAllocatorThread() {
    createVSpace();
    createCSpace();
    loadElf();
    setupStack();
    createIPCBuffer();
    setupTCB();
    setupRegisters();
    setupCSlots();
}

static void resumeMemoryAllocatorThread() {
    seL4_Error error = seL4_TCB_Resume(GLOBALS_ASSORTED_CSLOT(memory_allocator_tcb));
    assert(error == seL4_NoError, error);
}

void Setup::launchMemoryAllocatorThread() {
    setupMemoryAllocatorThread();
    seL4_DebugDumpScheduler();
    resumeMemoryAllocatorThread();
}