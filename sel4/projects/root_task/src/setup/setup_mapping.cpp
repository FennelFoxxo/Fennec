#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"
#include "mapping.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <utils/util.h>
}

static seL4_CPtr current_mapping_cap_cptr;

static bool getMappingCSlotFunc(seL4_CPtr* cptr) {
    retErrorIfFail(current_mapping_cap_cptr != GLOBALS_ASSORTED_CSLOT(mapping_caps_end), "Ran out of mapping caps!");
    *cptr = current_mapping_cap_cptr++;
    return true;
}

bool Setup::setupMapping() {
    current_mapping_cap_cptr = GLOBALS_ASSORTED_CSLOT(mapping_caps_start);
    
    Globals::mapping_context = MappingContext(seL4_CapInitThreadVSpace, MemTree::getFreeUntyped, MemTree::returnUsedUntyped,
                                              getMappingCSlotFunc, {0, 0, GLOBALS_CSLOT_INDEX(temp_slot), PAGE_CNODE_BITS});
    
    
    return true;
}