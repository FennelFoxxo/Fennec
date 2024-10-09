#include "setup.h"

#include "globals/globals.h"

extern "C" {
#include <sel4platsupport/bootinfo.h>
}

bool Setup::initBootInfo() {
    Globals::boot_info = platsupport_get_bootinfo();
    
    Globals::num_empty_slots = Globals::boot_info->empty.end - Globals::boot_info->empty.start;
	retErrorIfFail(Globals::num_empty_slots >= GLOBALS_MIN_EMPTY_SLOTS, "Not enough empty slots");
	
	Globals::bootstrap_empty_start = Globals::boot_info->empty.start;
    
    return true;
}