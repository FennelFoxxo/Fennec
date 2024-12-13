#include "setup.h"

#include "globals/globals.h"
#include "mem_tree.hpp"

extern "C" {
#include <sel4/sel4.h>
#include <utils/util.h>
}

static seL4_CPtr current_mapping_cap_cptr;

static bool tryIncrementMappingCapCPtr() {
    current_mapping_cap_cptr++;
    return current_mapping_cap_cptr != GLOBALS_ASSORTED_CSLOT(mapping_caps_end);
}

static bool retypeWrapper(seL4_Word offset, seL4_Word type) {
    retErrorIfFail(Globals::num_memory_chunks != Globals::memory_chunks_next_available, "Ran out of untyped caps while setting up mapping");
    
    seL4_CPtr untyped_cptr, return_cptr;
    retFalseIfFail(MemTree::getFreeUntyped(&untyped_cptr, &return_cptr));
    
    // Retype from provided slot
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, type, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), PAGE_CNODE_BITS, offset, 1);
    retErrorIfFail(error == seL4_NoError, "Failed to retype while setting up mapping");
    
    retFalseIfFail(MemTree::returnUsedUntyped(return_cptr));

    return true;
}

static bool mapPDPT(seL4_Word vaddr) {
    seL4_CPtr saved_mapping_cap = current_mapping_cap_cptr;
    retErrorIfFail(tryIncrementMappingCapCPtr(), "Ran out of mapping caps while setting up bootstrap mapping");

    retFalseIfFail(retypeWrapper(saved_mapping_cap - GLOBALS_CSLOT(assorted_caps), seL4_X86_PDPTObject));
    
    seL4_Error error = seL4_X86_PDPT_Map(saved_mapping_cap, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page directory page table");
    
    return true;
}

static bool mapPD(seL4_Word vaddr) {
    seL4_CPtr saved_mapping_cap = current_mapping_cap_cptr;
    retErrorIfFail(tryIncrementMappingCapCPtr(), "Ran out of mapping caps while setting up bootstrap mapping");

    retFalseIfFail(retypeWrapper(saved_mapping_cap - GLOBALS_CSLOT(assorted_caps), seL4_X86_PageDirectoryObject));
    
    seL4_Error error = seL4_X86_PageDirectory_Map(saved_mapping_cap, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    
    if (error == seL4_NoError) return true; // Success, all done
    retErrorIfFail(error == seL4_FailedLookup, "Failed to map x86 page directory"); // Failed lookup is ok, we just need to map more structures
    
    if (!mapPDPT(vaddr)) return false;
    
    // Try mapping again
    error = seL4_X86_PageDirectory_Map(saved_mapping_cap, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page directory");
    
    return true;
}

static bool mapPT(seL4_Word vaddr) {
    seL4_CPtr saved_mapping_cap = current_mapping_cap_cptr;
    retErrorIfFail(tryIncrementMappingCapCPtr(), "Ran out of mapping caps while setting up bootstrap mapping");

    retFalseIfFail(retypeWrapper(saved_mapping_cap - GLOBALS_CSLOT(assorted_caps), seL4_X86_PageTableObject));
    
    seL4_Error error = seL4_X86_PageTable_Map(saved_mapping_cap, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    
    if (error == seL4_NoError) return true; // Success, all done
    retErrorIfFail(error == seL4_FailedLookup, "Failed to map x86 page table"); // Failed lookup is ok, we just need to map more structures
    
    if (!mapPD(vaddr)) return false;
    
    // Try mapping again
    error = seL4_X86_PageTable_Map(saved_mapping_cap, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page table");
    
    return true;
}

bool Setup::setupMapping() {
    current_mapping_cap_cptr = GLOBALS_ASSORTED_CSLOT(mapping_caps_start);
    
    for (seL4_Word vaddr = BOOTSTRAP_VADDR_START; vaddr < BOOTSTRAP_VADDR_END; vaddr += BIT(21)) {
        retFalseIfFail(mapPT(vaddr));
    }
    return true;
}