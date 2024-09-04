#include "memory.h"

extern "C" {
#include <stdio.h>
#include <utils/util.h>
}


static bool must_allocate_new_L2;
static bool must_allocate_new_L1;

static seL4_Word precalc_total_memory_chunks_count;
static seL4_Word precalc_usable_memory_chunks_count; // Total memory chunks minus chunks used for L2 and L1 tables

// Globals are used everywhere in here and it's annoying to pass them to every function, so this is set in setupMemory and used as a shorthand in all the other functions
static Globals::Globals* globals_ptr; 


seL4_Word calcL2Index() { return globals_ptr->num_memory_chunks / BIT(GLOBALS_CNODE_BITS); }
seL4_Word calcL1Index() { return globals_ptr->num_memory_chunks % BIT(GLOBALS_CNODE_BITS); }
seL4_UntypedDesc* getRegionDesc(seL4_CPtr untyped_ptr) { return &globals_ptr->boot_info->untypedList[untyped_ptr - globals_ptr->boot_info->untyped.start]; }
bool isAcceptableRegion(seL4_UntypedDesc* desc) { return !desc->isDevice && desc->sizeBits >= GLOBALS_LARGE_CHUNK_BITS && desc->paddr > 0xD00000; } // Only non-device memory at least LARGE_CHUNK in size

// Calculate how many total and usable chunks we have, before doing any retyping
void precalcChunkCount() {
	for (seL4_CPtr slot = globals_ptr->boot_info->untyped.start; slot != globals_ptr->boot_info->untyped.end; slot++) { // Iterate over list of untypeds provided by bootinfo
		seL4_UntypedDesc* desc = getRegionDesc(slot);
		if (isAcceptableRegion(desc)) precalc_total_memory_chunks_count += BIT(desc->sizeBits - GLOBALS_LARGE_CHUNK_BITS);
	}

	// Calculates how many usable memory chunks there are, based on the fact that #L2 + #L1 + usable = total, and usable <= #L1 * #cslots <= usable + #cslots
	seL4_Word num_cslots = BIT(GLOBALS_CNODE_BITS);
	precalc_usable_memory_chunks_count = (num_cslots * (precalc_total_memory_chunks_count - 1)) / (num_cslots + 1);
}


bool allocateL2CNode(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype(region_to_use, seL4_CapTableObject, GLOBALS_CNODE_BITS, seL4_CapInitThreadCNode, 0, 0, globals_ptr->L2_memory_slot, 1);
	return error == seL4_NoError;
}

bool allocateL1Cnode(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype(region_to_use, seL4_CapTableObject, GLOBALS_CNODE_BITS, seL4_CapInitThreadCNode, globals_ptr->L2_memory_slot, seL4_WordBits, calcL2Index(), 1);
	return error == seL4_NoError;
}

bool allocateLargeChunk(seL4_CPtr region_to_use) {
	seL4_Error error = seL4_Untyped_Retype(region_to_use, seL4_UntypedObject, GLOBALS_LARGE_CHUNK_BITS, globals_ptr->L2_memory_slot, calcL2Index(), GLOBALS_CNODE_BITS, calcL1Index(), 1);
	return error == seL4_NoError;
}

// Break a specific region into large chunks
bool breakRegionIntoChunks(seL4_CPtr untyped_ptr) {
	seL4_Word chunks_left = BIT(getRegionDesc(untyped_ptr)->sizeBits - GLOBALS_LARGE_CHUNK_BITS);
	while (chunks_left > 0) {
		chunks_left--;
		
		// If the L2 index is higher than the last slot of the L2 CNode, then we've mapped all memory!
		if (calcL2Index() == BIT(GLOBALS_CNODE_BITS)) return true;
		
		// Allocate new L2 slot if needed and continue to next iteration (should only need to be done once)
		if (must_allocate_new_L2) {
			retFalseIfFail(allocateL2CNode(untyped_ptr));
			must_allocate_new_L2 = false;
			continue;
		}
		
		// Allocate new L1 slot if needed and continue to next iteration
		if (must_allocate_new_L1) {
			retFalseIfFail(allocateL1Cnode(untyped_ptr));
			must_allocate_new_L1 = false;
			continue;
		}
		
		if (globals_ptr->num_memory_chunks % 100 == 0) {
			printf("  Mapped %lu memory chunks out of %lu...\n", globals_ptr->num_memory_chunks, precalc_usable_memory_chunks_count);
		}

		// Allocate 1MB chunk from untyped memory
		retFalseIfFail(allocateLargeChunk(untyped_ptr));
		globals_ptr->num_memory_chunks++;
		
		// If L1 index has overflowed back to the start, we'll need to allocate a new L1 CNode
		if (calcL1Index() == 0) {
			must_allocate_new_L1 = true;
		}
		
	}
	return true;
}


// Breaks every region into LARGE_CHUNK-sized chunks
bool breakRegionsIntoChunks() {
	precalcChunkCount();
	retFalseIfFail(	precalc_total_memory_chunks_count >= GLOBALS_MIN_LARGE_CHUNKS	); // Exit if not enough chunks
	must_allocate_new_L2 = true;
	must_allocate_new_L1 = true;
	
	for (seL4_CPtr slot = globals_ptr->boot_info->untyped.start; slot != globals_ptr->boot_info->untyped.end; slot++) { // Iterate over list of untypeds provided by bootinfo
		seL4_UntypedDesc* desc = getRegionDesc(slot);
		if (isAcceptableRegion(desc)) {
			retFalseIfFail(	breakRegionIntoChunks(slot)	);
		}
	}
	retFalseIfFail( precalc_usable_memory_chunks_count == globals_ptr->num_memory_chunks ); // Something must have gone horribly wrong!
	
	printf("Mapped all %lu memory chunks\n", globals_ptr->num_memory_chunks);
	
	return true;
}

bool reserveBootstrapMemory() {
	seL4_Error error = seL4_CNode_Copy(	seL4_CapInitThreadCNode, globals_ptr->bootstrap_memory_slot, seL4_WordBits,		// Destination
										globals_ptr->L2_memory_slot, 0, GLOBALS_CNODE_BITS * 2, seL4_AllRights);		// Source
	globals_ptr->memory_chunks_allocable_start++;
	return error == seL4_NoError;
}

bool Setup::setupMemory(Globals::Globals& globals) {
	globals_ptr = &globals;
	retFalseIfFail( breakRegionsIntoChunks() );
	retFalseIfFail( reserveBootstrapMemory() );
	
	return true;
}
