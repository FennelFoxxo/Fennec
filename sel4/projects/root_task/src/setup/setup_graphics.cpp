#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <utils/util.h>
}

static seL4_CPtr current_graphics_ctable = GLOBALS_CSLOT_INDEX(graphics_ctables_start);
static seL4_Word free_graphics_cslot_offset = 0;

// Returns true if ok to continue, false if end of graphics ctables reached
static bool outOfGraphicsCTables() {
    return (current_graphics_ctable == GLOBALS_CSLOT_INDEX(graphics_ctables_end)) ? true : false;
}

// Returns true if ok to continue, false if end of graphics caps reached or error occurred
static bool retypeIntoCTable() {
    seL4_CPtr untyped_cptr, return_cptr;
    seL4_Error error;
    
    retErrorIfFail(!outOfGraphicsCTables(), "Ran out of graphics ctables while setting up graphics!");
    retFalseIfFail(MemTree::getFreeUntyped(&untyped_cptr, &return_cptr));

    // Retype from provided slot and put in temp slot
    error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(bootstrap_caps), PAGE_CNODE_BITS, GLOBALS_BOOTSTRAP_CSLOT(temp_slot), 1);
    retErrorIfFail(error == seL4_NoError, "Failed to retype new ctable while setting up graphics!");

    retFalseIfFail(MemTree::returnUsedUntyped(return_cptr));
    
    // Mutate from temp slot so that the inner cslots are accessible at the correct depth
    error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, current_graphics_ctable, PAGE_CNODE_BITS,
                              seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(temp_slot), seL4_WordBits, seL4_WordBits - 2 * PAGE_CNODE_BITS);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate new ctable while setting up graphics!");
    
    return true;
}

// Performs a retype on the provided region, automatically inserting the cap to the new untyped into
// the correct graphics CNode. Returns the address to this location in untyped_cptr (optional, may be nullptr)
// Returns true on success
static bool retypeWrapper(seL4_CPtr region, seL4_Word type, seL4_Word size_bits, seL4_CPtr* untyped_cptr) {
    // If the current CTable is full, we need to create a new one
    if (free_graphics_cslot_offset == BIT(PAGE_CNODE_BITS)) {
        current_graphics_ctable++; // Make space for the CTable to be placed in
        retFalseIfFail(retypeIntoCTable());
        free_graphics_cslot_offset = 0;
    }
    seL4_Error error = seL4_Untyped_Retype(region, type, size_bits,
                                           seL4_CapInitThreadCNode, current_graphics_ctable, PAGE_CNODE_BITS, free_graphics_cslot_offset, 1);
    retErrorIfFail(error == seL4_NoError, "Failed to retype while setting up graphics!");
    
    if (untyped_cptr != nullptr) *untyped_cptr = (current_graphics_ctable << (seL4_WordBits - PAGE_CNODE_BITS)) | free_graphics_cslot_offset;
    free_graphics_cslot_offset++;
    
    return true;
}

// If some device memory starts at phys addr e.x. 0x5000 but you want to map the portion at 0x6000,
// you need to first retype 0x1000 bytes as some dummy object, so that the untyped watermark moves up
// to 0x6000 and then you can start allocating frames. This function creates those dummy objects so that
// the next retype operation will start at target_addr
static bool allocatePadding(seL4_CPtr untyped, seL4_Word start_addr, seL4_Word target_addr) {
    seL4_Word diff = target_addr - start_addr;
    
    retErrorIfFail((diff & 0xfff) == 0, "start_addr and target_addr must be aligned to a page!");
    
    for (int i = 63; i != 12; i--) {
        if (diff & BIT(i)) {
            retFalseIfFail(retypeWrapper(untyped, seL4_UntypedObject, i, nullptr));
        }
    }
    return true;
}

static seL4_UntypedDesc* getRegionDesc(seL4_CPtr untyped_ptr) {
    return &Globals::boot_info->untypedList[untyped_ptr - Globals::boot_info->untyped.start];
}

static bool mapFramesFromRegion(seL4_CPtr slot, seL4_Word target_paddr, seL4_Word target_vaddr, seL4_Word bytes_to_map) {
    seL4_UntypedDesc* desc = getRegionDesc(slot);
    seL4_Word start_addr = desc->paddr;
    
    allocatePadding(slot, start_addr, target_paddr);
    
    for (seL4_Word i = 0; i < bytes_to_map/BIT(seL4_PageBits); i++) {
        seL4_CPtr frame_cptr;

        retFalseIfFail(retypeWrapper(slot, seL4_X86_4K, 0, &frame_cptr));

        retErrorIfFail(Globals::mapping_context.mapFrame(frame_cptr, target_vaddr), "setupGraphics() page map failed!");
        
        target_vaddr += BIT(seL4_PageBits);
    }
    return true;
}

bool Setup::setupGraphics() {
    // If no framebuffer data was provided, we can't set up graphics
    retErrorIfFail(Globals::framebuffer_info != nullptr, "No framebuffer info found!");
    
    // Create a single CTable to start with
    retFalseIfFail(retypeIntoCTable());
    
    // Framebuffer length in bytes
    seL4_Word graphics_buffer_len = Globals::framebuffer_info->pitch * Globals::framebuffer_info->height;
    
    seL4_Word graphics_paddr = Globals::framebuffer_info->addr;
    seL4_Word graphics_vaddr = GRAPHICS_VADDR;
    
    // Round up to nearest page
    seL4_Word bytes_left_to_map = ((graphics_buffer_len + BIT(seL4_PageBits) - 1) / BIT(seL4_PageBits)) * BIT(seL4_PageBits);
    
    // Iterate over every untyped region to find which ones contain the memory mapped framebuffer frames
    for (seL4_CPtr slot = Globals::boot_info->untyped.start; slot != Globals::boot_info->untyped.end; slot++) {
        seL4_UntypedDesc* desc = getRegionDesc(slot);
        seL4_Word start_addr = desc->paddr;
        seL4_Word region_length = BIT(desc->sizeBits);
        seL4_Word end_addr = start_addr + region_length;
        
        // If the paddr we're looking for is within the region under consideration
        if (start_addr <= graphics_paddr && end_addr > graphics_paddr) {
            // It's possible that the region is larger than the bytes left needed to map, or that we need
            // to map more bytes than are in the region. Use whichever is smaller to avoid going past bounds
            seL4_Word bytes_to_map = MIN(bytes_left_to_map, region_length);
            retFalseIfFail(mapFramesFromRegion(slot, graphics_paddr, graphics_vaddr, bytes_to_map));
            
            bytes_left_to_map -= bytes_to_map;
            graphics_paddr += bytes_to_map;
            graphics_vaddr += bytes_to_map;
        }
        // If no more bytes left to map, then we're done!
        if (bytes_left_to_map == 0) {
            break;
        }
    }
    
    return true;
}