#pragma once

extern "C" {
#include <sel4/sel4.h>
}

namespace MemTree {
    // Moves the next free untyped into a cslot at a depth of seL4_WordBits
    // so that it is accessible for retype operations. Returns the cslot location
    // in untyped_cptr. Once the untyped is used, it can be returned back to its
    // original location, which is returned in return_index (save this value somewhere)
    // Returns true on success
    bool getFreeUntyped(seL4_CPtr* untyped_cptr, seL4_CPtr* return_cptr);
    
    // Given the return_index provided by getFreeUntyped(), this function
    // moves the untyped back into the memory tree, so that whatever temporary
    // spot it occupied is now free
    bool returnUsedUntyped(seL4_CPtr return_cptr);
}