#include "memalloc.h"
#include "globals/macros.h"

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
}

#define calcRootCSlotAddress(cslot_index) ((seL4_Word)cslot_index << (seL4_WordBits - GLOBALS_CNODE_BITS))

namespace MemAlloc {
	
	bool start() {
		seL4_Error error = seL4_TCB_Resume(Globals::memory_allocator_tcb_slot);
		if (error != seL4_NoError) return false;

		return true;
	}

	void thread(seL4_Word arg) {
		
		printf("Thread started successfully!\n");
		
		printf("Attempting to suspend thread...\n");
		seL4_TCB_Suspend(calcRootCSlotAddress(THREAD_TCB_SLOT));
		
		printf("This should not print!!\n");
	}
}