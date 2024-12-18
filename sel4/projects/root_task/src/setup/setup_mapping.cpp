#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"
#include "mapping.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <utils/util.h>
}

static seL4_CPtr next_free_paging_cslot;

static bool getMappingCSlotFunc(seL4_CPtr* cptr) {
    retErrorIfFail(next_free_paging_cslot != GLOBALS_ASSORTED_CSLOT(paging_caps_end), "Ran out of paging caps!");
    *cptr = next_free_paging_cslot++;
    return true;
}

bool Setup::setupMapping() {
    next_free_paging_cslot = GLOBALS_ASSORTED_CSLOT(paging_caps_start);
    
    Globals::mapping_context = MappingContext(seL4_CapInitThreadVSpace, MemTree::getFreeUntyped, MemTree::returnUsedUntyped,
                                              getMappingCSlotFunc, {0, 0, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS});
    
    
    return true;
}