#pragma once

extern "C" {
#include <sel4/sel4.h>
}

// A class for abstracting away the logic of creating paging objects.
// Handles creating page tables, page directories, etc behind
// the scenes.

// When an untyped cap is needed, get_untyped_func will be called, which
// should return a cptr to an untyped cap in cptr, and optionally return
// a return-cptr in ret_cptr. When the untyped has been used, ret_untyped_func
// will be called with the return-cptr previously returned. If the return callback
// is not needed, ret_cptr may be null.

// get_cslot_func is used in a similar way, but for when a free
// cslot is needed to place the paging objects created by a retype operation.

// All callback functions should return true on success, and false on failure, in
// which case the paging will be aborted

// It also needs a temporary slot for placing the retyped objects in before they
// are moved to a free cslot. The location should be placed in the TempSlot object

class MappingContext {
    typedef void (*GetUntypedFunc)(seL4_CPtr* cptr, seL4_CPtr* ret_cptr);
    typedef void (*RetUntypedFunc)(seL4_CPtr ret_cptr);
    typedef void (*GetCSlotFunc)(seL4_CPtr* cptr);
    
    struct TempSlot {
        seL4_Word node_index;
        seL4_Word node_depth;
        seL4_Word node_offset;
        seL4_Word node_size_bits;
    };
    
public:
    MappingContext();
    
    MappingContext(seL4_CPtr vspace,
                   GetUntypedFunc get_untyped_func, RetUntypedFunc ret_untyped_func,
                   GetCSlotFunc get_cslot_func, TempSlot temp_slot);
    
    bool mapFrame(seL4_CPtr frame, seL4_Word vaddr);
    bool unmapFrame(seL4_CPtr frame);
                   
private:
    
    bool mapPagingStructure(seL4_Word type, seL4_Word vaddr);
    
    seL4_CPtr vspace;
    GetUntypedFunc get_untyped_func;
    RetUntypedFunc ret_untyped_func;
    GetCSlotFunc get_cslot_func;
    TempSlot temp_slot;
};