#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4/sel4.h>
}

bool Setup::setupNewCSpace() {
    // Create new croot object
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_BOOTSTRAP_CSLOT(bootstrap_memory), seL4_CapTableObject, GLOBALS_SMALL_CNODE_BITS,
                                           seL4_CapInitThreadCNode, 0, 0, GLOBALS_BOOTSTRAP_CSLOT(croot), 1);
    retErrorIfFail(error == seL4_NoError, "Failed to create new root cnode while creating new croot");
    
    // Move bootstrap croot into new root caps cnode object, setting guard so that the original root caps are still accessed at a depth of wordsize
    error = seL4_CNode_Mutate(GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_CSLOT_INDEX(bootstrap_caps), GLOBALS_SMALL_CNODE_BITS,
                              seL4_CapInitThreadCNode, seL4_CapInitThreadCNode, seL4_WordBits, seL4_WordBits - GLOBALS_SMALL_CNODE_BITS - Globals::boot_info->initThreadCNodeSizeBits);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate bootstrap croot while creating new croot");
    
    // Move new croot into where old croot was, such that seL4_CapInitThreadCNode will point to this new croot
    error = seL4_CNode_Move(GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_CapInitThreadCNode, seL4_WordBits,
                            GLOBALS_BOOTSTRAP_CSLOT(croot), GLOBALS_BOOTSTRAP_CSLOT(croot), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to replace original croot slot while creating new croot");
    
    // Change thread croot and set guard to 0
    error = seL4_TCB_SetSpace(seL4_CapInitThreadTCB, seL4_CapNull, seL4_CapInitThreadCNode, 0, seL4_CapInitThreadVSpace, 0);
    retErrorIfFail(error == seL4_NoError, "Failed to set new croot while creating new croot");
    
    
    // Create assorted caps cnode
    error = seL4_Untyped_Retype(GLOBALS_BOOTSTRAP_CSLOT(bootstrap_memory), seL4_CapTableObject, GLOBALS_SMALL_CNODE_BITS,
                                seL4_CapInitThreadCNode, 0, 0, GLOBALS_CSLOT_INDEX(temp_slot), 1);
    retErrorIfFail(error == seL4_NoError, "Failed to create assorted caps cnode while creating new croot");
    
    // Mutate assorted caps cnode to have wordsize guard
    error = seL4_CNode_Mutate(seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS,
                              seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(temp_slot), GLOBALS_SMALL_CNODE_BITS, seL4_WordBits - 2*GLOBALS_SMALL_CNODE_BITS);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate assorted caps cnode while creating new croot");
    
    // Move bootstrap memory into assorted slots cnode
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_WordBits,
                            seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(bootstrap_memory), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to move bootstrap memory while creating new croot");
    
    // Move original L2 memory into new croot
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(L2_memory), seL4_WordBits,
                              seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(L2_memory), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to mutate L2 memory cnode while creating new croot");
    
    // Move memory allocator chunk into new croot
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, GLOBALS_ASSORTED_CSLOT(memory_allocator_chunk), seL4_WordBits,
                            seL4_CapInitThreadCNode, GLOBALS_BOOTSTRAP_CSLOT(memory_allocator_chunk), seL4_WordBits);
    retErrorIfFail(error == seL4_NoError, "Failed to move memory allocator memory while creating new croot");
    
    return true;
}