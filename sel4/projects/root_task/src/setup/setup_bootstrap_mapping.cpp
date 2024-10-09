#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4/sel4.h>
}

static seL4_Error mapPDPT() {
    return seL4_X86_PDPT_Map(GLOBALS_ASSORTED_CSLOT(bootstrap_pdpt), seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR, seL4_X86_Default_VMAttributes);
}

static seL4_Error mapPD() {
    return seL4_X86_PageDirectory_Map(GLOBALS_ASSORTED_CSLOT(bootstrap_pd), seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR, seL4_X86_Default_VMAttributes);
}

static seL4_Error mapPT() {
    return seL4_X86_PageTable_Map(GLOBALS_ASSORTED_CSLOT(bootstrap_pt), seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR, seL4_X86_Default_VMAttributes);
}

bool Setup::setupBootstrapMapping() {
    // Create page table object
    seL4_Error error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_X86_PageTableObject, 0,
                                           seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS, GLOBALS_ASSORTED_CSLOT_INDEX(bootstrap_pt), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into x86 page table object");
    
    // Attempt to map page table
    error = mapPT();
    if (error == seL4_NoError) return true; // Mapped page table ok, all done
    retErrorIfFail(error == seL4_FailedLookup, "Failed to map x86 page table"); // Failed lookup is ok, we just need to map more structures
    
    // Create page directory object
	 error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_X86_PageDirectoryObject, 0,
                                 seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS, GLOBALS_ASSORTED_CSLOT_INDEX(bootstrap_pd), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into x86 page directory object");
	
    error = mapPD();
    if (error == seL4_NoError) {
        // No error, let's try mapping page table again
        error = mapPT();
        retErrorIfFail(error == seL4_NoError, "Failed to map x86 page table");
        return true;
    }
    retErrorIfFail(error == seL4_FailedLookup, "Failed to map x86 page directory"); // Failed lookup is ok, we just need to map more structures
	
	// Create page directory page table object
	 error = seL4_Untyped_Retype(GLOBALS_ASSORTED_CSLOT(bootstrap_memory), seL4_X86_PDPTObject, 0,
                                 seL4_CapInitThreadCNode, GLOBALS_CSLOT_INDEX(assorted_caps), GLOBALS_SMALL_CNODE_BITS, GLOBALS_ASSORTED_CSLOT_INDEX(bootstrap_pdpt), 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into x86 page directory page table object");
    
    error = mapPDPT();
    // At this point it's an error if mapping failed again
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page directory page table");

    // Let's try mapping page directory again
    error = mapPD();
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page directory");
    // And now map page table
    error = mapPT();
    retErrorIfFail(error == seL4_NoError, "Failed to map x86 page table");
    
    return true;
}