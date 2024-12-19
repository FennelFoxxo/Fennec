#include "mem_tree.hpp"

#include "globals/globals.h"
#include "assert.h"

void MemTree::getFreeUntyped(seL4_CPtr* untyped_cptr, seL4_CPtr* return_cptr) {
    if (Globals::has_memory_tree_been_moved) {
        // After the memory tree has been moved, all the untypeds inside are already accessible
        // and so we don't need to move them - just return the location of the cslot
        
        // TODO
        
        Globals::memory_chunks_next_available++;
        return;
    }
    
    // Move next free untyped into temp slot
    seL4_Error error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot), seL4_WordBits,
                                       GLOBALS_BOOTSTRAP_CSLOT(l5_memory), Globals::memory_chunks_next_available, PAGE_CNODE_BITS * 5);
    assert(error == seL4_NoError, error);
    
    *untyped_cptr = GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot);
    *return_cptr = Globals::memory_chunks_next_available++;
}

void MemTree::returnUsedUntyped(seL4_CPtr return_cptr) {
    if (Globals::has_memory_tree_been_moved) {
        // After the memory tree has been moved, all the untypeds inside are already accessible
        // and so getFreeUntyped() doesn't need to move them, which means nothing needs to be
        // returned back to its original position
        return;
    }
    
    // Move untyped back into L5 tree, freeing up the temp slot
    seL4_Error error = seL4_CNode_Move(GLOBALS_BOOTSTRAP_CSLOT(l5_memory), return_cptr, PAGE_CNODE_BITS * 5,
                                       seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(mem_tree_temp_slot), seL4_WordBits);
    assert(error == seL4_NoError, error);
}