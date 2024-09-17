#include "setup.h"

#include "globals/globals.h"
#include "memory.h"

extern "C" {
#include <utils/util.h>
}

bool setBootInfo() {
	Globals::boot_info = platsupport_get_bootinfo();
	return true;
}

bool setCptrs() {
	Globals::num_empty_slots = Globals::boot_info->empty.end - Globals::boot_info->empty.start;
	retErrorIfFail(Globals::num_empty_slots >= GLOBALS_MIN_EMPTY_SLOTS, "Not enough empty slots");
	
	seL4_CPtr empty_ptr = Globals::boot_info->empty.start;
	Globals::bootstrap_memory_slot			= empty_ptr++;
	Globals::L2_memory_slot					= empty_ptr++;
	Globals::page_directory_slot			= empty_ptr++;
	Globals::page_table_slot				= empty_ptr++;
	
	Globals::memory_allocator_tcb_slot		= empty_ptr++;
	Globals::memory_allocator_stack_slot	= empty_ptr++;
	Globals::memory_allocator_croot_slot	= empty_ptr++;
	Globals::memory_allocator_ipc_slot		= empty_ptr++;
	Globals::memory_allocator_tls_slot		= empty_ptr++;
	
	return true;
}

bool setupGlobals() {
	retFalseIfFail(	setBootInfo());
	retFalseIfFail(	setCptrs());
	return true;
}


bool Setup::setup() {
	retFalseIfFail(	setupGlobals());
	retFalseIfFail(	setupMemory());
	//retFalseIfFail(	setupMemAllocThread());
	
	return true;
}