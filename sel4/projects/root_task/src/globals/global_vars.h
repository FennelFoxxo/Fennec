#pragma once

#include "macros.h"
#include "mapping.hpp"

#include <stdint.h>

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

// As chunks of memory are used up in the creation of objects during the bootstrapping process,
// this variable is the index of the next unused chunk
extern seL4_Word memory_chunks_next_available;

// Once all the memory has been chunked and a new cspace set up, the memory tree
// is moved and its guard mutated so that all the untypeds are accessible at a depth
// of seL4_WordBits. Code that relies on the memory tree must be made aware about
// whether it has been moved yet
extern bool has_memory_tree_been_moved;

// Object for abstracting away the logic of creating paging objects
extern MappingContext mapping_context;

// CSlots, initialized by setup_cptrs() - actual allocations are done by various allocate functions
extern seL4_CPtr bootstrap_empty_start; // Start of empty cslots

struct MultibootFrameBuffer {
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  type;
} __attribute__((packed));

extern MultibootFrameBuffer* framebuffer_info;

// Slot locations in initial root task cnode - offsets from bootstrap_empty_start. Only used until new croot is set
enum class BootstrapSlots {
    temp_slot,              // Temporary slot to place new objects before mutating them
    mem_tree_temp_slot,     // Temporary slot that MemTree's getFreeUntyped() and returnUsedUntyped() uses
    l5_memory,              // Chunk of memory to be turned into a 5-level cnode structure to hold page-sized untypeds
    croot,                  // New cnode to set as the root task croot

    end
};

enum class Slots {
    bootstrap_caps = 0,
    temp_slot,                  // Another temporary slot to place new objects before mutating them
    assorted_caps,
    l5_memory,                  // Final location of L5 tree
    memory_allocator_frames,    // CNode of frames used by memory allocator thread

    // Caps used to hold cnodes for setting up graphics
    graphics_ctables_start,
    graphics_ctables_end = graphics_ctables_start + GLOBALS_MIN_CHUNKS_SETUP_GRAPHICS
};

enum class AssortedSlots {
    bootstrap_memory,
    memory_allocator_chunk,
    
    // Caps used for storing paging structures (page table, page directory, etc)
    paging_caps_start,
    paging_caps_end = paging_caps_start + GLOBALS_MIN_CHUNKS_SETUP_MAPPING,
    
    thread_temp_frame,  // Temporary page used for copying data into thread address space

    memory_allocator_tcb,
    memory_allocator_croot,
    memory_allocator_vspace,
    memory_allocator_ipc_buffer,
    
    // Room for paging structures to be created for mapping memory allocator in thread vspace
    memory_allocator_paging_objects_start,
    memory_allocator_paging_objects_end = memory_allocator_paging_objects_start + 16 // This endpoint is inclusive

};

extern char memory_allocator_stack[1024] __attribute__((aligned(16)));

}