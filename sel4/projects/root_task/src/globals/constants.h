#pragma once

// Memory region constants
#define GLOBALS_MAX_USABLE_MEMORY_REGIONS 100
#define GLOBALS_LARGE_CHUNK_BITS 20
#define GLOBALS_SMALL_CHUNK_BITS 12 // Page size

// Required minimums
#define GLOBALS_MIN_CHUNKS_SETUP_NEW_CSPACE 2
#define GLOBALS_MIN_CHUNKS_SETUP_MAPPING 16
#define GLOBALS_MIN_CHUNKS_SETUP_GRAPHICS 16
#define GLOBALS_MIN_LARGE_CHUNKS 10

// Log2 of how many cslots can fit in a small chunk
#define GLOBALS_SMALL_CNODE_BITS (GLOBALS_SMALL_CHUNK_BITS - seL4_SlotBits)

// Log2 of how many cslots can fit in a large chunk
#define GLOBALS_LARGE_CNODE_BITS (GLOBALS_LARGE_CHUNK_BITS - seL4_SlotBits)

// Log2 of how many cslots can fit in a page
#define PAGE_CNODE_BITS (seL4_PageBits - seL4_SlotBits)

// Log2 of how many chunks should be processed at once during the memory chunking process
// Should be <= PAGE_CNODE_BITS
#define MEMORY_CHUNKING_BATCH_SIZE PAGE_CNODE_BITS

// Log2 of how many cslots can fit in the memory allocator frame table, which holds the physical memory needed to start the memory allocator thread
// Should be able to map memory allocator elf file
#define GLOBALS_MEMORY_ALLOCATOR_FRAMES_CNODE_BITS (GLOBALS_LARGE_CHUNK_BITS - GLOBALS_SMALL_CHUNK_BITS)

// CSlot locations
#define THREAD_CNODE_SLOT 1 // Index of where cspace root should be placed in spawned thread's cnode
#define THREAD_TCB_SLOT 2
#define THREAD_IPC_BUFFER_SLOT 3 // CSlot that points to IPC buffer
#define THREAD_EXTRA_SLOTS 20	// Index where extra caps are placed into spawned thread's cnode

#define MAX_THREAD_EXTRA_SLOTS 20

#define MEMORY_ALLOCATOR_PRIORITY 255


// --- ROOT TASK MEMORY LAYOUT ---

// Addresses for where to place various objects needed during bootstrapping process
#define BOOTSTRAP_VADDR_START 0x40000000 // 1GB
#define BOOTSTRAP_VADDR_END 0x40400000 // 1GB + 4MB

#define TEMP_FRAME_VADDR (BOOTSTRAP_VADDR_START + 0x0000) // Temporary page used for writing data before remapping into another address space

#define GRAPHICS_VADDR (BOOTSTRAP_VADDR_START + 0x1000) // Address where graphics memory is mapped

// --- SPAWNED THREADS MEMORY LAYOUT ---

#define THREAD_STACK_TOP_VADDR 0x7fffa000
#define THREAD_IPC_BUFFER_VADDR 0x7fffc000
#define THREAD_TLS_VADDR 0x7ffff000


/*
#define MEM_ALLOC_STACK_VADDR 0x2000
#define MEM_ALLOC_IPC_BUFFER_VADDR 0x4000
#define MEM_ALLOC_TLS_VADDR 0x6000
#define MEM_ALLOC_TEXT_VADDR 0x8000*/