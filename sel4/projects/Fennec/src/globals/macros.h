#pragma once

// Memory region constants
#define GLOBALS_MAX_USABLE_MEMORY_REGIONS 100
#define GLOBALS_LARGE_CHUNK_BITS 20
#define GLOBALS_SMALL_CHUNK_BITS 12

// Required minimums
#define GLOBALS_MIN_LARGE_CHUNKS 10
#define GLOBALS_MIN_EMPTY_SLOTS 10

// Number of bits to address any spawned thread's root cnode. Also equal to the (log 2) number of cslots in the cnode
//#define GLOBALS_CNODE_BITS (GLOBALS_SMALL_CHUNK_BITS - seL4_SlotBits)
#define GLOBALS_CNODE_BITS 12

// --- MEMALLOC CONSTANTS ---

// CSlot locations
#define MEM_ALLOC_SLOT_CNODE 1 // Index of cspace root
#define MEM_ALLOC_SLOT_L2_MEM_CNODE 2 // CSlot that points to L2 memory chunk CNode, which contains all system memory broken into 1MB chunks
#define MEM_ALLOC_SLOT_IPC_BUFFER 3 // CSlot that points to IPC buffer
#define MEM_ALLOC_PRIORITY 250


// --- MEMORY LAYOUT ---

// Addresses for where to place various objects needed during bootstrapping process
#define BOOTSTRAP_VADDR 0x40000000 // 1GB
#define MEM_ALLOC_IPC_BUFFER_VADDR (BOOTSTRAP_VADDR + 0x0000)
#define MEM_ALLOC_TLS_VADDR (MEM_ALLOC_IPC_BUFFER_VADDR + 0x1000)

// Useful macros

#define GLOBALS_LARGE_CHUNK_SIZE BIT(GLOBALS_LARGE_CHUNK_BITS)
#define GLOBALS_SMALL_CHUNK_SIZE BIT(GLOBALS_SMALL_CHUNK_BITS)

#define _Static_assert static_assert // Needed for sel4runtime to compile

#define retFalseIfFail(f) if (!(f)) return false