#include "mapping.hpp"

#include <sel4/sel4_arch/mapping.h>

MappingContext::MappingContext() {}

MappingContext::MappingContext(seL4_CPtr vspace,
                               GetUntypedFunc get_untyped_func, RetUntypedFunc ret_untyped_func,
                               GetCSlotFunc get_cslot_func, TempSlot temp_slot) :
                                    vspace(vspace),
                                    get_untyped_func(get_untyped_func),
                                    ret_untyped_func(ret_untyped_func),
                                    get_cslot_func(get_cslot_func),
                                    temp_slot(temp_slot) {
}



bool MappingContext::mapPagingStructure(seL4_Word type, seL4_Word vaddr) {
    seL4_CPtr untyped_cptr, untyped_ret_cptr;
    
    // Get untyped
    get_untyped_func(&untyped_cptr, &untyped_ret_cptr);

    // Retype into object
    seL4_Error error = seL4_Untyped_Retype(untyped_cptr, type, 0,
                                           seL4_CapInitThreadCNode, temp_slot.node_index, temp_slot.node_depth, temp_slot.node_offset, 1);
	if (error != seL4_NoError) return false;
    
    // Return untyped
    if (ret_untyped_func) {
        ret_untyped_func(untyped_ret_cptr);
    }
    
    // Get free cslot
    seL4_CPtr cslot_cptr;
    get_cslot_func(&cslot_cptr);

    // Move retyped object from temp slot to free cslot
    error = seL4_CNode_Move(seL4_CapInitThreadCNode, cslot_cptr, seL4_WordBits,
                            seL4_CapInitThreadCNode, (temp_slot.node_index << temp_slot.node_size_bits) | temp_slot.node_offset, temp_slot.node_depth + temp_slot.node_size_bits);
    if (error != seL4_NoError) return false;

    switch(type) {
        case seL4_X86_PageTableObject:
            error = seL4_X86_PageTable_Map(cslot_cptr, vspace, vaddr, seL4_X86_Default_VMAttributes);
            break;
        case seL4_X86_PageDirectoryObject:
            error = seL4_X86_PageDirectory_Map(cslot_cptr, vspace, vaddr, seL4_X86_Default_VMAttributes);
            break;
        case seL4_X86_PDPTObject:
            error = seL4_X86_PDPT_Map(cslot_cptr, vspace, vaddr, seL4_X86_Default_VMAttributes);
            break;
    }
    
    if (error != seL4_NoError) return false;
    return true;
}

 

bool MappingContext::mapFrame(seL4_CPtr frame, seL4_Word vaddr) {
    seL4_Error error = seL4_X86_Page_Map(frame, vspace, vaddr,
                                         seL4_ReadWrite, seL4_X86_Default_VMAttributes);

    if (error == seL4_NoError) return true;

    // Failed lookup is ok, we just need to map more page objects, but any other return code is an error
	if (error != seL4_FailedLookup) return false;
    
    seL4_Word failed_level = seL4_MappingFailedLookupLevel();
    
    // Need to map PDPT
    if (failed_level >= 39) {
        if (!mapPagingStructure(seL4_X86_PDPTObject, vaddr)) return false;
    }
    
    // Need to map PD
    if (failed_level >= 30) {
        if (!mapPagingStructure(seL4_X86_PageDirectoryObject, vaddr)) return false;
    }
    
    // Need to map PT
    if (failed_level >= 21) {
        if (!mapPagingStructure(seL4_X86_PageTableObject, vaddr)) return false;
    }
    
    // Retry mapping
    error = seL4_X86_Page_Map(frame, vspace, vaddr,
                              seL4_ReadWrite, seL4_X86_Default_VMAttributes);
	return error == seL4_NoError;
}

bool MappingContext::unmapFrame(seL4_CPtr frame) {
    seL4_Error error = seL4_X86_Page_Unmap(frame);
    return error == seL4_NoError;
}