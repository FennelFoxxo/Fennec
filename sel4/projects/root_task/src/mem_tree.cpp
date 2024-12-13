#include "mem_tree.hpp"

#include "globals/globals.h"

bool MemTree::getFreeUntyped(seL4_CPtr* untyped_cptr, seL4_CPtr* return_cptr) {
    if (Globals::has_memory_tree_been_moved) {
        // After the memory tree has been moved, all the untypeds inside are already accessible
        // and so we don't need to move them - just return the location of the cslot
        
        // TODO
        
        Globals::memory_chunks_next_available++;
        return true;
    }
    
    // Move next free untyped into temp slot
    seL4_Error error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot), seL4_WordBits,
                                       GLOBALS_BOOTSTRAP_CSLOT(l5_memory), Globals::memory_chunks_next_available, PAGE_CNODE_BITS * 5);
    retErrorIfFail(error == seL4_NoError, "Failed to move untyped from L5 tree into temp slot");
    
    *untyped_cptr = GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot);
    *return_cptr = Globals::memory_chunks_next_available++;
    
    return true;
}

bool MemTree::returnUsedUntyped(seL4_CPtr return_cptr) {
    if (Globals::has_memory_tree_been_moved) {
        // After the memory tree has been moved, all the untypeds inside are already accessible
        // and so getFreeUntyped() doesn't need to move them, which means nothing needs to be
        // returned back to its original position
        return true;
    }
    
    // Move untyped back into L5 tree, freeing up the temp slot
    seL4_Error error = seL4_CNode_Move(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), return_cptr, PAGE_CNODE_BITS * 5,
                                       seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to move untyped from temp slot into L5 tree");
    
    return true;
}