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

// CSlots, initialized by setup_cptrs() - actual allocations are done by various allocate functions
seL4_Word num_empty_slots = 0;


seL4_Word num_memory_chunks = 0; // Total number of usable memory chunks

// Some chunks of memory are needed to spin up initial threads - this is the index to the first free chunk after those, that can be dynamically allocated to other threads
seL4_Word memory_chunks_allocable_start = 0;

seL4_CPtr L2_memory_slot = 0;
seL4_CPtr bootstrap_memory_slot = 0; // LARGE_CHUNK_SIZE-sized block of memory for initial bootstrapping tasks

// Memory allocator

seL4_CPtr memory_allocator_tcb_slot = 0;
seL4_CPtr memory_allocator_croot_slot = 0;

char memory_allocator_stack[1024] __attribute__((aligned(16)));

};

}