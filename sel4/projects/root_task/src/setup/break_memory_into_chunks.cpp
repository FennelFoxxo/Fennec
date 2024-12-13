#include "setup.h"

#include "globals/globals.h"

#include "graphics.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <stdio.h>
#include <utils/util.h>
}

static seL4_CPtr current_region = 0;
static seL4_Word bytes_left_in_region = 0;
static seL4_Word total_chunks = 0;
static seL4_Word processed_chunks = 0;

static bool is_new_cpsace_setup = false;
static bool is_mapping_setup = false;
static bool is_graphics_setup = false;

static int last_chunk_update = 0;
static int last_loading_bar_x_pos = 0;

// Called when seL4_Untyped_Retype succeeds - handles debug output, gui update, etc
static void onRetypeSuccess() {
    if (processed_chunks - last_chunk_update > 10000) {
        printf("  Processed %lu chunks out of %lu\n", processed_chunks, total_chunks);
        last_chunk_update = processed_chunks;
    }
    
    int curr_x_pos = getWidth() * processed_chunks / total_chunks;
    if (is_graphics_setup && curr_x_pos != last_loading_bar_x_pos) {
        rect(last_loading_bar_x_pos, 0, curr_x_pos - last_loading_bar_x_pos, 25, {255, 255, 255});
        last_loading_bar_x_pos = curr_x_pos;
    }
}

// Called when a new usable chunk is available
// Returns true if no errors occur during this function
static bool onNewUsableChunkSuccess() {
    
    seL4_Word num_free_chunks = Globals::num_memory_chunks - Globals::memory_chunks_next_available;
    
    if (num_free_chunks >= GLOBALS_MIN_CHUNKS_SETUP_NEW_CSPACE && !is_new_cpsace_setup) {
        retFalseIfFail(Setup::setupNewCSpace());
        is_new_cpsace_setup = true;
    }
    
    if (num_free_chunks >= GLOBALS_MIN_CHUNKS_SETUP_MAPPING && is_new_cpsace_setup && !is_mapping_setup) {
        retFalseIfFail(Setup::setupMapping());
        is_mapping_setup = true;
    }
    
    if (num_free_chunks >= GLOBALS_MIN_CHUNKS_SETUP_GRAPHICS && is_mapping_setup && !is_graphics_setup) {
        retFalseIfFail(Setup::setupGraphics());
        is_graphics_setup = true;
    }
    
    return true;
}



// At the start, none of the tiers exist yet, so every one needs to be allocated
static bool allocate_new_L1 = true;
static bool allocate_new_L2 = true;
static bool allocate_new_L3 = true;
static bool allocate_new_L4 = true;
static bool allocate_new_L5 = true;

// Since the CTable structure is set up as a 5-tiered "pyramid" shape, every slot at the bottom
// can be reached by descending from the CTable at the top, through every layer to the bottom.
// In other words, at each tier, you can pick which CSlot to follow to the next lower tier.
// For example, with 8 CSlots per CTable, the 1000th CSlot in the structure can be reached at
// L5 index 0, L4 index 1, L3 index 7, L2 index 5, L1 index 0.
// These functions calculate the index combination to get to the next free CSlot
static seL4_Word calcL1Index() { return Globals::num_memory_chunks % BIT(PAGE_CNODE_BITS); }
static seL4_Word calcL2Index() { return (Globals::num_memory_chunks / BIT(PAGE_CNODE_BITS * 1)) % BIT(PAGE_CNODE_BITS); }
static seL4_Word calcL3Index() { return (Globals::num_memory_chunks / BIT(PAGE_CNODE_BITS * 2)) % BIT(PAGE_CNODE_BITS); }
static seL4_Word calcL4Index() { return (Globals::num_memory_chunks / BIT(PAGE_CNODE_BITS * 3)) % BIT(PAGE_CNODE_BITS); }
static seL4_Word calcL5Index() { return (Globals::num_memory_chunks / BIT(PAGE_CNODE_BITS * 4)) % BIT(PAGE_CNODE_BITS); }

// Get description (size and is device) from untyped
static seL4_UntypedDesc* getRegionDesc(seL4_CPtr untyped_ptr) {
    return &Globals::boot_info->untypedList[untyped_ptr - Globals::boot_info->untyped.start];
}

// Returns true for non-device memory that is at least as big as a page
static bool isAcceptableRegion(seL4_CPtr region) {
    seL4_UntypedDesc* desc = getRegionDesc(region);
    return !desc->isDevice && desc->sizeBits >= seL4_PageBits;
}

static seL4_Word calcTotalChunks() {
    seL4_Word total_chunks = 0;
    for (seL4_CPtr region = Globals::boot_info->untyped.start; region != Globals::boot_info->untyped.end; region++) {
        if (isAcceptableRegion(region)) {
            total_chunks += BIT(getRegionDesc(region)->sizeBits - seL4_PageBits);
        }
    }
    return total_chunks;
}

// Returns CPtr of first acceptable region after start (inclusive) - returns 0 if none found
static seL4_CPtr findAcceptableRegion(seL4_CPtr start) {
    for (seL4_CPtr region = start; region != Globals::boot_info->untyped.end; region++) {
        if (isAcceptableRegion(region)) {
            return region;
        }
    }
    // Reached end of regions without finding an acceptable one
    return 0;
}


// Set flags for if new tables must be allocated
static void setTableAllocationFlags() {
    // We can test if new cnodes need to be allocated by temporarily increasing the
    // chunk count and seeing if it would cause any indexes to overflow. For example,
    // if a node has 10 slots and we just filled the 9th one, then 9+1 (mod 10) overflows
    // to 0 and we know we need another node
    Globals::num_memory_chunks += BIT(MEMORY_CHUNKING_BATCH_SIZE);
    
    // The index would only be 0 if it overflowed, so if it's != 0 then it didn't overflow
    // If it didn't overflow, then none of the upper tiers could have overflowed either, so we can exit early
    if (calcL1Index() != 0) goto done;
    allocate_new_L1 = true;
    
    if (calcL2Index() != 0) goto done;
    allocate_new_L2 = true;
        
    if (calcL3Index() != 0) goto done;
    allocate_new_L3 = true;
            
    if (calcL4Index() != 0) goto done;
    allocate_new_L4 = true;
                
    if (calcL5Index() != 0) goto done;
    allocate_new_L5 = true;
    
    done:
    // Return chunk count to how it was before
    Globals::num_memory_chunks -= BIT(MEMORY_CHUNKING_BATCH_SIZE);
}

// Returns true if allocation succeeded.
// Allocation could fail for two reasons - either retype failed (sel4_error != seL4_NoError), or we're out of memory
// If allocation failed but error is 0, then we've chunked all available memory
static bool retypeWrapper(seL4_CNode root, seL4_Word index, seL4_Word depth, seL4_Word offset, seL4_Word type, seL4_Error* sel4_error, seL4_Word num_obj) {
    while (num_obj != 0) {
        if (bytes_left_in_region == 0) {
            // This region is all empty, start search from the next region
            current_region = findAcceptableRegion(current_region+1);
            if (current_region == 0) { // No acceptable region found, we've processed all memory
                *sel4_error = seL4_NoError; // Indicate that allocation failure was not due to an seL4 retype() error
                return false;
            }
            bytes_left_in_region = BIT(getRegionDesc(current_region)->sizeBits);
        }
        
        // The meaning of size_bits when retyping is different for a CNode vs untyped
        seL4_Word size_bits = (type == seL4_CapTableObject ? PAGE_CNODE_BITS : seL4_PageBits);

        // The region might not be big enough to retype all the requested objects at once
        seL4_Word num_obj_capped = MIN(num_obj, bytes_left_in_region / BIT(size_bits));
        
        *sel4_error = seL4_Untyped_Retype(current_region, type, size_bits,
                                          root, index, depth, offset, num_obj_capped);
                                          
        retErrorIfFail(*sel4_error == seL4_NoError, "Failed to retype while chunking memory!");
        
        // No matter what size_bits is, the total object size is the same (1 page)
        bytes_left_in_region -= num_obj_capped * BIT(seL4_PageBits);
        
        num_obj -= num_obj_capped;
        offset += num_obj_capped; // Shift over offset for next loop
        processed_chunks += num_obj_capped;

        onRetypeSuccess();
    }
    return true;
}

static bool retypeIntoCTable(seL4_CNode root, seL4_Word index, seL4_Word depth, seL4_Word offset, seL4_Error* sel4_error) {
    return retypeWrapper(root, index, depth, offset, seL4_CapTableObject, sel4_error, 1);
}

static bool retypeIntoFrames(seL4_CNode root, seL4_Word index, seL4_Word depth, seL4_Word offset, seL4_Error* sel4_error, seL4_Word num_obj) {
    // Increment memory chunks counter since we're creating a new chunk here
    Globals::num_memory_chunks += num_obj;
    return retypeWrapper(root, index, depth, offset, seL4_UntypedObject, sel4_error, num_obj);
}

static bool allocateL5(seL4_Error* sel4_error) {
    // There can only ever be one L5 table, which is allocated at the start.
    // If we've already allocated one (we can check this by seeing if chunks have already started being allocated),
    // Then we have no more room to store frames and we're all done!
    if (Globals::num_memory_chunks != 0) {
        return false;
    }
    return retypeIntoCTable(seL4_CapInitThreadCNode, 0, 0, GLOBALS_BOOTSTRAP_CSLOT(l5_memory), sel4_error);
}

static bool allocateL4(seL4_Error* sel4_error) {
    if (allocate_new_L5) {
        if (!allocateL5(sel4_error)) return false;
        allocate_new_L5 = false;
    }
    return retypeIntoCTable(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), 0, 0, calcL5Index(), sel4_error);
}

static bool allocateL3(seL4_Error* sel4_error) {
    if (allocate_new_L4) {
        if (!allocateL4(sel4_error)) return false;
        allocate_new_L4 = false;
    }
    return retypeIntoCTable(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), calcL5Index(), PAGE_CNODE_BITS, calcL4Index(), sel4_error);
}

static bool allocateL2(seL4_Error* sel4_error) {
    if (allocate_new_L3) {
        if (!allocateL3(sel4_error)) return false;
        allocate_new_L3 = false;
    }
    
    seL4_Word index = 0;
    index |= (calcL5Index() << PAGE_CNODE_BITS * 1);
    index |= (calcL4Index() << PAGE_CNODE_BITS * 0);
    
    return retypeIntoCTable(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), index, PAGE_CNODE_BITS * 2, calcL3Index(), sel4_error);
}

static bool allocateL1(seL4_Error* sel4_error) {
    if (allocate_new_L2) {
        if (!allocateL2(sel4_error)) return false;
        allocate_new_L2 = false;
    }
    
    seL4_Word index = 0;
    index |= (calcL5Index() << PAGE_CNODE_BITS * 2);
    index |= (calcL4Index() << PAGE_CNODE_BITS * 1);
    index |= (calcL3Index() << PAGE_CNODE_BITS * 0);
    
    return retypeIntoCTable(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), index, PAGE_CNODE_BITS * 3, calcL2Index(), sel4_error);
}



static bool allocateFrame(seL4_Error* sel4_error) {
    if (allocate_new_L1) {
        if (!allocateL1(sel4_error)) return false;
        allocate_new_L1 = false;
    }
    
    seL4_Word index = 0;
    index |= (calcL5Index() << PAGE_CNODE_BITS * 3);
    index |= (calcL4Index() << PAGE_CNODE_BITS * 2);
    index |= (calcL3Index() << PAGE_CNODE_BITS * 1);
    index |= (calcL2Index() << PAGE_CNODE_BITS * 0);
    
    // See if this frame allocation used up all the cslots in the cnode, and more tables need to be allocated
    setTableAllocationFlags();
    
    return retypeIntoFrames(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), index, PAGE_CNODE_BITS * 4, calcL1Index(), sel4_error, BIT(MEMORY_CHUNKING_BATCH_SIZE));
}

bool Setup::breakMemoryIntoChunks() {
    // Start by skipping to first acceptable region
    current_region = findAcceptableRegion(Globals::boot_info->untyped.start);
    // Make sure at least one acceptable region was found
    retErrorIfFail(current_region != 0, "No acceptable regions found for retyping!");
    
    bytes_left_in_region = BIT(getRegionDesc(current_region)->sizeBits);
    
    total_chunks = calcTotalChunks();
    
    seL4_Error sel4_error;
    
    while (allocateFrame(&sel4_error)) {
        retFalseIfFail(onNewUsableChunkSuccess());
    }
    
    if (sel4_error != seL4_NoError) return false;
    /*
    // Move L5 tree and set guard so that all the untyped caps are accessible
    sel4_error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(l5_memory), PAGE_CNODE_BITS,
                                   seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(l5_memory), seL4_WordBits, seL4_WordBits - PAGE_CNODE_BITS*6);
    retErrorIfFail(sel4_error == seL4_NoError, "Failed to mutate L5 memory while chunking memory");
    
    Globals::has_memory_tree_been_moved = true;*/
    
    printf("%lu chunks processed, %lu chunks available\n", processed_chunks, Globals::num_memory_chunks);
    
    return true;
}