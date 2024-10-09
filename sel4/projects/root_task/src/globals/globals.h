#pragma once

#include "macros.h"

extern "C" {
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
}

namespace Globals {

// Boot info permanent pointer
extern seL4_BootInfo* boot_info;

// Error message pointer
extern const char* error_msg;

// Info about available memory
extern seL4_Word num_empty_slots;
extern seL4_Word num_memory_chunks; // Total number of usable memory chunks

// Some chunks of memory are needed to spin up initial threads - this is the index to the first free chunk after those, that can be dynamically allocated to other threads
extern seL4_Word memory_chunks_allocable_start;


// CSlots, initialized by setup_cptrs() - actual allocations are done by various allocate functions
extern seL4_CPtr bootstrap_empty_start; // Start of empty cslots

// Slot locations in initial root task cnode - offsets from bootstrap_empty_start. Only used until new croot is set
enum class BootstrapSlots {
    L2_memory,              // Chunk of memory to be turned into a 2-level cnode structure to hold large chunk untypeds
    bootstrap_memory,       // LARGE_CHUNK_SIZE-sized block of memory for initial bootstrapping tasks
    memory_allocator_chunk, // Chunk of data to use for mapping memory allocator thread
    croot,                  // New cnode to set as the root task croot
       
    
};

enum class Slots {
    bootstrap_caps = 0,
    assorted_caps,
    memory_allocator_frames, // CNode of frames used by memory allocator thread
    temp_slot // Temp slot to place new objects before mutating them
};

enum class AssortedSlots {
    bootstrap_memory,
    L2_memory,
    memory_allocator_chunk,
    
    bootstrap_pdpt,
    bootstrap_pd,
    bootstrap_pt,
    
    thread_temp_frame, // Temporary page used for copying data into thread address space

    memory_allocator_tcb,
    memory_allocator_croot,
    memory_allocator_vspace,
    memory_allocator_ipc_buffer,
    
    // Room for paging structures to be created for mapping memory allocator in thread vspace
    memory_allocator_paging_objects_start,
    memory_allocator_paging_objects_end = memory_allocator_paging_objects_start + 16, // This endpoint is inclusive
};

extern char memory_allocator_stack[1024] __attribute__((aligned(16)));

}