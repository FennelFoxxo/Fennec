#pragma once

#include "globals/globals.h"

namespace MemAlloc {
	bool start(Globals::Globals& globals);
	
	void thread(void* arg);
}