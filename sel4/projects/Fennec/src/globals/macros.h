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
//#define MEM_ALLOC_SLOT_IPC_BUFFER 3 // CSlot that points to IPC buffer
#define MEM_ALLOC_PRIORITY 250




#define TEMP_IPC_ADDR 0x7000000

// Useful macros

#define GLOBALS_LARGE_CHUNK_SIZE BIT(GLOBALS_LARGE_CHUNK_BITS)
#define GLOBALS_SMALL_CHUNK_SIZE BIT(GLOBALS_SMALL_CHUNK_BITS)

#define retFalseIfFail(f) if (!(f)) return false