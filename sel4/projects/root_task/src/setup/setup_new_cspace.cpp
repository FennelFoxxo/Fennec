#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"
#include "assert.h"

extern "C" {
#include <sel4/sel4.h>
#include <stdio.h>
}

static void retypeNewCNode(seL4_Word dest_offset) {
    seL4_CPtr untyped_cptr, return_cptr;
    MemTree::getFreeUntyped(&untyped_cptr, &return_cptr);

    // Retype from temp slot
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, dest_offset, 1);
    assert(error == seL4_NoError, error);
    
    MemTree::returnUsedUntyped(return_cptr);
}

static void retypeNewRootCNode() {
    retypeNewCNode(GLOBALS_BOOTSTRAP_CSLOT(croot));
}

static void mutateOriginalRootCNode() {
    seL4_Error error = seL4_CNode_Mutate(GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_CSLOT_INDEX(bootstrap_caps), PAGE_CNODE_BITS,
                                         seL4_CapInitThreadCNode, seL4_CapInitThreadCNode, seL4_WordBits, seL4_WordBits - PAGE_CNODE_BITS - Globals::boot_info->initThreadCNodeSizeBits);
    assert(error == seL4_NoError, error);
}

static void replaceCapInitThreadCNode() {
    seL4_Error error = seL4_CNode_Move(GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_CapInitThreadCNode, seL4_WordBits,
                                       GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_WordBits);
    assert(error == seL4_NoError, error);
}

static void updateThreadCSpace() {
    seL4_Error error = seL4_TCB_SetSpace(seL4_CapInitThreadTCB, seL4_CapNull, seL4_CapInitThreadCNode, 0, seL4_CapInitThreadVSpace, 0);
    assert(error == seL4_NoError, error);
}

static void setupAssortedCapsCNode() {
    retypeNewCNode(GLOBALS_CSLOT_INDEX(temp_slot));
    
    // Mutate assorted caps cnode so that cslots are addressed at a depth of seL4_WordBits
    seL4_Error error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                         seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS, seL4_WordBits - 2 * PAGE_CNODE_BITS);
    assert(error == seL4_NoError, error);
}

void Setup::setupNewCSpace() {
    // Create new cnode object to use as new root
    retypeNewRootCNode();
    
    // Move original croot into new root caps cnode object, setting guard so that the original root caps
    // can still be accessed through the same cptrs as before
    mutateOriginalRootCNode();
    
    // Move new root cnode into where old root cnode was, such that seL4_CapInitThreadCNode will point to this new croot
    replaceCapInitThreadCNode();
    
    // Change thread croot and set guard to 0
    updateThreadCSpace();
    
    // Create assorted caps cnode and set guard
    setupAssortedCapsCNode();
}