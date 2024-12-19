#pragma once

// CSlot locations
#define GLOBALS_BOOTSTRAP_INDEX(name) (static_cast<seL4_CPtr>(Globals::BootstrapSlots::name))
#define GLOBALS_BOOTSTRAP_CSLOT(name) (Globals::bootstrap_empty_start + GLOBALS_BOOTSTRAP_INDEX(name))

#define GLOBALS_CSLOT_INDEX(name) (static_cast<seL4_CPtr>(Globals::Slots::name))
#define GLOBALS_CSLOT(name) (GLOBALS_CSLOT_INDEX(name) << (seL4_WordBits - PAGE_CNODE_BITS))

#define GLOBALS_ASSORTED_CSLOT_INDEX(name) (static_cast<seL4_CPtr>(Globals::AssortedSlots::name))
#define GLOBALS_ASSORTED_CSLOT(name) (GLOBALS_CSLOT(assorted_caps) | GLOBALS_ASSORTED_CSLOT_INDEX(name))

// Other useful macros
#define GLOBALS_LARGE_CHUNK_SIZE BIT(GLOBALS_LARGE_CHUNK_BITS)
#define GLOBALS_SMALL_CHUNK_SIZE BIT(GLOBALS_SMALL_CHUNK_BITS)

#define _Static_assert static_assert // Needed for sel4runtime to compile