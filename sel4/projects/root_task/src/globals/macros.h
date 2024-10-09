#pragma once

// Memory region constants
#define GLOBALS_MAX_USABLE_MEMORY_REGIONS 100
#define GLOBALS_LARGE_CHUNK_BITS 20
#define GLOBALS_SMALL_CHUNK_BITS 12 // Page size

// Required minimums
#define GLOBALS_MIN_LARGE_CHUNKS 10
#define GLOBALS_MIN_EMPTY_SLOTS 20

// Log2 of how many cslots can fit in a small chunk
#define GLOBALS_SMALL_CNODE_BITS (GLOBALS_SMALL_CHUNK_BITS - seL4_SlotBits)

// Log2 of how many cslots can fit in a large chunk
#define GLOBALS_LARGE_CNODE_BITS (GLOBALS_LARGE_CHUNK_BITS - seL4_SlotBits)

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


// --- MEMORY LAYOUT ---

// Addresses for where to place various objects needed during bootstrapping process
#define BOOTSTRAP_VADDR 0x40000000 // 1GB

#define TEMP_FRAME_VADDR (BOOTSTRAP_VADDR + 0x0000) // Temporary page used for writing data before remapping into another address space

#define THREAD_STACK_TOP_VADDR 0x7fffa000
#define THREAD_IPC_BUFFER_VADDR 0x7fffc000
#define THREAD_TLS_VADDR 0x7ffff000

/*
#define MEM_ALLOC_STACK_VADDR 0x2000
#define MEM_ALLOC_IPC_BUFFER_VADDR 0x4000
#define MEM_ALLOC_TLS_VADDR 0x6000
#define MEM_ALLOC_TEXT_VADDR 0x8000*/

// CSlot locations
#define GLOBALS_BOOTSTRAP_INDEX(name) (static_cast<seL4_CPtr>(Globals::BootstrapSlots::name))
#define GLOBALS_BOOTSTRAP_CSLOT(name) (Globals::bootstrap_empty_start + GLOBALS_BOOTSTRAP_INDEX(name))

#define GLOBALS_CSLOT_INDEX(name) (static_cast<seL4_CPtr>(Globals::Slots::name))
#define GLOBALS_CSLOT(name) (GLOBALS_CSLOT_INDEX(name) << (seL4_WordBits - GLOBALS_SMALL_CNODE_BITS))

#define GLOBALS_ASSORTED_CSLOT_INDEX(name) (static_cast<seL4_CPtr>(Globals::AssortedSlots::name))
#define GLOBALS_ASSORTED_CSLOT(name) (GLOBALS_CSLOT(assorted_caps) | GLOBALS_ASSORTED_CSLOT_INDEX(name))

// Other useful macros
#define GLOBALS_LARGE_CHUNK_SIZE BIT(GLOBALS_LARGE_CHUNK_BITS)
#define GLOBALS_SMALL_CHUNK_SIZE BIT(GLOBALS_SMALL_CHUNK_BITS)

#define GLOBALS_GET_ERROR() (Globals::error_msg)
#define GLOBALS_SET_ERROR(msg) (Globals::error_msg = msg)

#define _Static_assert static_assert // Needed for sel4runtime to compile

#define retFalseIfFail(f) if (!(f)) return false;
#define retErrorIfFail(f, msg) if (!(f)) {GLOBALS_SET_ERROR(msg); return false;}
#define retIfFail(f) if (!(f)) return;