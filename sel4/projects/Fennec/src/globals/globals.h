#pragma once

#include "macros.h"

extern "C" {
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
}

namespace Globals {

// Boot info permanent pointer
extern seL4_BootInfo* boot_info;

// Info about available memory
extern seL4_Word num_empty_slots;
extern seL4_Word num_memory_chunks; // Total number of usable memory chunks

// Some chunks of memory are needed to spin up initial threads - this is the index to the first free chunk after those, that can be dynamically allocated to other threads
extern seL4_Word memory_chunks_allocable_start;

// CSlots, initialized by setup_cptrs() - actual allocations are done by various allocate functions
extern seL4_CPtr bootstrap_memory_slot; // LARGE_CHUNK_SIZE-sized block of memory for initial bootstrapping tasks
extern seL4_CPtr L2_memory_slot;		// Chunk of memory to be turned into a 2-level cnode structure to hold large chunk untypeds
extern seL4_CPtr page_directory_slot;
extern seL4_CPtr page_table_slot;

// Memory allocator

extern seL4_CPtr memory_allocator_tcb_slot;
extern seL4_CPtr memory_allocator_croot_slot;
extern seL4_CPtr memory_allocator_ipc_buffer_slot;
extern seL4_CPtr memory_allocator_tls_slot; // This is under the memory allocator section for organization, but it doesn't need to be passed to the thread

extern char memory_allocator_stack[1024] __attribute__((aligned(16)));

}