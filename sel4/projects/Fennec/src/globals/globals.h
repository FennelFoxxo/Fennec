#pragma once

#include "macros.h"

extern "C" {
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
}

namespace Globals {

struct Globals {

// Initializes global variables
bool setup();

// Boot info permanent pointer
seL4_BootInfo* boot_info = nullptr;

// Info about available memory
seL4_Word num_empty_slots = 0;
seL4_Word num_memory_chunks = 0; // Total number of usable memory chunks

// Some chunks of memory are needed to spin up initial threads - this is the index to the first free chunk after those, that can be dynamically allocated to other threads
seL4_Word memory_chunks_allocable_start = 0;

// CSlots, initialized by setup_cptrs() - actual allocations are done by various allocate functions
seL4_CPtr bootstrap_memory_slot = 0; // LARGE_CHUNK_SIZE-sized block of memory for initial bootstrapping tasks
seL4_CPtr L2_memory_slot = 0;		// Chunk of memory to be turned into a 2-level cnode structure to hold large chunk untypeds
seL4_CPtr page_directory_slot = 0;
seL4_CPtr page_table_slot = 0;

// Memory allocator

seL4_CPtr memory_allocator_tcb_slot = 0;
seL4_CPtr memory_allocator_croot_slot = 0;
seL4_CPtr memory_allocator_ipc_buffer_slot = 0;
seL4_CPtr memory_allocator_tls_slot = 0; // This is under the memory allocator section for organization, but it doesn't need to be passed to the thread

char memory_allocator_stack[1024] __attribute__((aligned(16)));

};

}