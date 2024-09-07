#pragma once

#include "globals/globals.h"

namespace MemAlloc {
	bool start();
	
	void thread(seL4_Word arg);
}