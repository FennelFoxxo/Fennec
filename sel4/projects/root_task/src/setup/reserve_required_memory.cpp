#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4/sel4.h>
}

static bool reserveBootstrapChunk() {
    // Use first memory chunk
	seL4_Error error = seL4_CNode_Move(	seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(bootstrap_memory), seL4_WordBits, // Destination
										GLOBALS_BOOTSTRAP_CSLOT(L2_memory), Globals::memory_chunks_allocable_start, GLOBALS_LARGE_CNODE_BITS * 2); // Source
	retErrorIfFail(error == seL4_NoError, "Failed to reserve bootstrap memory");
	Globals::memory_chunks_allocable_start++;
	return true;
}

static bool reserveMemoryAllocatorChunk() {
    // Use second memory chunk
	seL4_Error error = seL4_CNode_Move(	seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(memory_allocator_chunk), seL4_WordBits, // Destination
										GLOBALS_BOOTSTRAP_CSLOT(L2_memory), Globals::memory_chunks_allocable_start, GLOBALS_LARGE_CNODE_BITS * 2); // Source
	retErrorIfFail(error == seL4_NoError, "Failed to reserve memory allocator memory");
	Globals::memory_chunks_allocable_start++;
	return true;
}

bool Setup::reserveRequiredMemory() {
    retFalseIfFail(reserveBootstrapChunk());
    retFalseIfFail(reserveMemoryAllocatorChunk());
	return true;
}