#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4/sel4.h>
#include <stdio.h>
#include <utils/util.h>
}

static bool must_allocate_new_L2;
static bool must_allocate_new_L1;

static seL4_Word precalc_total_memory_chunks_count;
static seL4_Word precalc_usable_memory_chunks_count; // Total memory chunks minus chunks used for L2 and L1 tables

static seL4_Word calcL2Index() { return Globals::num_memory_chunks / BIT(GLOBALS_LARGE_CNODE_BITS); }
static seL4_Word calcL1Index() { return Globals::num_memory_chunks % BIT(GLOBALS_LARGE_CNODE_BITS); }
static seL4_UntypedDesc* getRegionDesc(seL4_CPtr untyped_ptr) { return &Globals::boot_info->untypedList[untyped_ptr - Globals::boot_info->untyped.start]; }

// Only non-device memory at least LARGE_CHUNK in size
static bool isAcceptableRegion(seL4_UntypedDesc* desc) { return !desc->isDevice && desc->sizeBits >= GLOBALS_LARGE_CHUNK_BITS; }


// Calculate how many total and usable chunks we have, before doing any retyping
static void precalcChunkCount() {
	for (seL4_CPtr slot = Globals::boot_info->untyped.start; slot != Globals::boot_info->untyped.end; slot++) { // Iterate over list of untypeds provided by bootinfo
		seL4_UntypedDesc* desc = getRegionDesc(slot);
		if (isAcceptableRegion(desc)) precalc_total_memory_chunks_count += BIT(desc->sizeBits - GLOBALS_LARGE_CHUNK_BITS);
	}

	// Calculates how many usable memory chunks there are, based on the fact that #L2 + #L1 + usable = total, and usable <= #L1 * #cslots <= usable + #cslots
	seL4_Word num_cslots = BIT(GLOBALS_LARGE_CNODE_BITS);
	precalc_usable_memory_chunks_count = (num_cslots * (precalc_total_memory_chunks_count - 1)) / (num_cslots + 1);
}


static bool allocateL2CNode(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype( region_to_use, seL4_CapTableObject, GLOBALS_LARGE_CNODE_BITS,
                                            seL4_CapInitThreadCNode, 0, 0, GLOBALS_BOOTSTRAP_CSLOT(L2_memory), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to allocate L2 CNode during memory setup");
	return true;
}

static bool allocateL1Cnode(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype( region_to_use, seL4_CapTableObject, GLOBALS_LARGE_CNODE_BITS,
                                            seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(L2_memory), seL4_WordBits, calcL2Index(), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to allocate L1 CNode during memory setup");
	return true;
}

static bool allocateLargeChunk(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype( region_to_use, seL4_UntypedObject, GLOBALS_LARGE_CHUNK_BITS,
                                            GLOBALS_BOOTSTRAP_CSLOT(L2_memory), calcL2Index(), GLOBALS_LARGE_CNODE_BITS, calcL1Index(), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to allocate chunk during memory setup");
	return true;
}

// Break a specific region into large chunks
static bool breakRegionIntoChunks(seL4_CPtr untyped_slot) {
	seL4_Word chunks_left = BIT(getRegionDesc(untyped_slot)->sizeBits - GLOBALS_LARGE_CHUNK_BITS);
	while (chunks_left > 0) {
		chunks_left--;
		
		// If the L2 index is higher than the last slot of the L2 CNode, then we've mapped all memory!
		if (calcL2Index() == BIT(GLOBALS_LARGE_CNODE_BITS)) return true;
		
		// Allocate new L2 slot if needed and continue to next iteration (should only need to be done once)
		if (must_allocate_new_L2) {
			retFalseIfFail(allocateL2CNode(untyped_slot));
			must_allocate_new_L2 = false;
			continue;
		}
		
		// Allocate new L1 slot if needed and continue to next iteration
		if (must_allocate_new_L1) {
			retFalseIfFail(allocateL1Cnode(untyped_slot));
			must_allocate_new_L1 = false;
			continue;
		}
		
		if (Globals::num_memory_chunks % 100 == 0) {
			printf("  Mapped %lu memory chunks out of %lu...\n", Globals::num_memory_chunks, precalc_usable_memory_chunks_count);
		}

		// Allocate 1MB chunk from untyped memory
		retFalseIfFail(allocateLargeChunk(untyped_slot));
		Globals::num_memory_chunks++;
		
		// If L1 index has overflowed back to the start, we'll need to allocate a new L1 CNode
		if (calcL1Index() == 0) {
			must_allocate_new_L1 = true;
		}
		
	}
	return true;
}


// Breaks every memory region into LARGE_CHUNK-sized chunks
bool Setup::breakMemoryIntoChunks() {
	precalcChunkCount();
	retErrorIfFail(precalc_total_memory_chunks_count >= GLOBALS_MIN_LARGE_CHUNKS, "Not enough chunks"); // Exit if not enough chunks
	must_allocate_new_L2 = true;
	must_allocate_new_L1 = true;
	
	for (seL4_CPtr slot = Globals::boot_info->untyped.start; slot != Globals::boot_info->untyped.end; slot++) { // Iterate over list of untypeds provided by bootinfo
		seL4_UntypedDesc* desc = getRegionDesc(slot);
		if (isAcceptableRegion(desc)) {
			retFalseIfFail(breakRegionIntoChunks(slot));
		}
	}
	retErrorIfFail(precalc_usable_memory_chunks_count == Globals::num_memory_chunks, "Failed to map as many chunks as expected"); // Something must have gone horribly wrong!
	
	printf("Mapped all %lu memory chunks\n", Globals::num_memory_chunks);
	
	return true;
}