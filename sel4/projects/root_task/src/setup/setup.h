#pragma once

#include "globals/globals.h"

namespace Setup {

bool initBootInfo();

bool breakMemoryIntoChunks();
bool reserveRequiredMemory();
bool setupNewCSpace();
bool setupBootstrapMapping();
bool launchMemoryAllocatorThread();

}