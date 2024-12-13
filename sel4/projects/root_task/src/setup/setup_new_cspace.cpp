#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <stdio.h>
}

static bool retypeNewCNode(seL4_Word dest_offset) {
    seL4_CPtr untyped_cptr, return_cptr;
    retFalseIfFail(MemTree::getFreeUntyped(&untyped_cptr, &return_cptr));

    // Retype from temp slot
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, seL4_CapTableObject, PAGE_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, dest_offset, 1);
    retErrorIfFail(error == seL4_NoError, "Failed to retype while setting up new cspace");
    
    retFalseIfFail(MemTree::returnUsedUntyped(return_cptr));

    return true;
}

static bool retypeNewRootCNode() {
    return retypeNewCNode(GLOBALS_BOOTSTRAP_CSLOT(croot));
}

static bool mutateOriginalRootCNode() {
    seL4_Error error = seL4_CNode_Mutate(GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_CSLOT_INDEX(bootstrap_caps), PAGE_CNODE_BITS,
                                         seL4_CapInitThreadCNode, seL4_CapInitThreadCNode, seL4_WordBits, seL4_WordBits - PAGE_CNODE_BITS - Globals::boot_info->initThreadCNodeSizeBits);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate original croot while creating new croot");
    return true;
}

static bool replaceCapInitThreadCNode() {
    seL4_Error error = seL4_CNode_Move(GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_CapInitThreadCNode, seL4_WordBits,
                                       GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to replace original croot slot while creating new croot");
    return true;
}

static bool updateThreadCSpace() {
    seL4_Error error = seL4_TCB_SetSpace(seL4_CapInitThreadTCB, seL4_CapNull, seL4_CapInitThreadCNode, 0, seL4_CapInitThreadVSpace, 0);
    retErrorIfFail(error == seL4_NoError, "Failed to set new croot while creating new croot");
    return true;
}

static bool setupAssortedCapsCNode() {
    retFalseIfFail(retypeNewCNode(GLOBALS_CSLOT_INDEX(temp_slot)));
    
    // Mutate assorted caps cnode so that cslots are addressed at a depth of seL4_WordBits
    seL4_Error error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS,
                                         seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS, seL4_WordBits - 2 * PAGE_CNODE_BITS);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate assorted caps cnode while creating new croot");
    return true;
}

bool Setup::setupNewCSpace() {
    // Create new cnode object to use as new root
    retFalseIfFail(retypeNewRootCNode());
    
    // Move original croot into new root caps cnode object, setting guard so that the original root caps
    // can still be accessed through the same cptrs as before
    retFalseIfFail(mutateOriginalRootCNode());
    
    // Move new root cnode into where old root cnode was, such that seL4_CapInitThreadCNode will point to this new croot
    retFalseIfFail(replaceCapInitThreadCNode());
    
    // Change thread croot and set guard to 0
    retFalseIfFail(updateThreadCSpace());
    
    // Create assorted caps cnode and set guard
    retFalseIfFail(setupAssortedCapsCNode());

    return true;
}