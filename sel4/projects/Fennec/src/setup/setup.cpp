#include "setup.h"

#include "globals/globals.h"
#include "memory.h"
#include "memalloc.h"

extern "C" {
#include <utils/util.h>
}

bool setBootInfo(Globals::Globals& globals) {
	globals.boot_info = platsupport_get_bootinfo();
	return true;
}

bool setCptrs(Globals::Globals& globals) {
	globals.num_empty_slots = globals.boot_info->empty.end - globals.boot_info->empty.start;
	if (globals.num_empty_slots < GLOBALS_MIN_EMPTY_SLOTS) return false; // Not enough empty cslots to satisfy requirements
	
	seL4_CPtr empty_ptr = globals.boot_info->empty.start;
	globals.bootstrap_memory_slot			= empty_ptr++;
	globals.L2_memory_slot					= empty_ptr++;
	globals.memory_allocator_tcb_slot		= empty_ptr++;
	globals.memory_allocator_croot_slot		= empty_ptr++;
	
	return true;
}

bool setupGlobals(Globals::Globals& globals) {
	retFalseIfFail(	setBootInfo(globals)	);
	retFalseIfFail(	setCptrs(globals)		);
	return true;
}


bool Setup::setup(Globals::Globals& globals) {
	retFalseIfFail(	setupGlobals(globals)	);
	retFalseIfFail(	setupMemory(globals)	);
	retFalseIfFail(	setupMemAllocThread(globals)	);
	
	return true;
}