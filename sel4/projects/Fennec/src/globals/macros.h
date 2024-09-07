#pragma once

// Memory region constants
#define GLOBALS_MAX_USABLE_MEMORY_REGIONS 100
#define GLOBALS_LARGE_CHUNK_BITS 20
#define GLOBALS_SMALL_CHUNK_BITS 12

// Required minimums
#define GLOBALS_MIN_LARGE_CHUNKS 10
#define GLOBALS_MIN_EMPTY_SLOTS 20

// Number of bits to address any spawned thread's root cnode. Also equal to the (log 2) number of cslots in the cnode
#define GLOBALS_CNODE_BITS (GLOBALS_SMALL_CHUNK_BITS - seL4_SlotBits)

// --- MEMALLOC CONSTANTS ---

// CSlot locations
#define THREAD_CNODE_SLOT 1 // Index of where cspace root should be placed in spawned thread's cnode
#define THREAD_TCB_SLOT 2
#define THREAD_IPC_BUFFER_SLOT 3 // CSlot that points to IPC buffer
#define THREAD_EXTRA_SLOTS 20	// Index where extra caps are placed into spawned thread's cnode

#define MEM_ALLOC_PRIORITY 250


// --- MEMORY LAYOUT ---

// Addresses for where to place various objects needed during bootstrapping process
#define BOOTSTRAP_VADDR 0x40000000 // 1GB


#define MEM_ALLOC_STACK_VADDR 0x0000
#define MEM_ALLOC_IPC_BUFFER_VADDR 0x2000
#define MEM_ALLOC_TLS_VADDR 0x4000

// Useful macros

#define GLOBALS_LARGE_CHUNK_SIZE BIT(GLOBALS_LARGE_CHUNK_BITS)
#define GLOBALS_SMALL_CHUNK_SIZE BIT(GLOBALS_SMALL_CHUNK_BITS)

#define GLOBALS_GET_ERROR() (Globals::error_msg)
#define GLOBALS_SET_ERROR(msg) (Globals::error_msg = msg)

#define _Static_assert static_assert // Needed for sel4runtime to compile

#define retFalseIfFail(f) if (!(f)) return false;
#define retErrorIfFail(f, msg) if (!(f)) {GLOBALS_SET_ERROR(msg); return false;}
#define retIfFail(f) if (!(f)) return;