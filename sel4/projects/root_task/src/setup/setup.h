#pragma once

#include "globals/globals.h"

namespace Setup {

bool initBootInfo();

bool breakMemoryIntoChunks();
bool setupMapping();
bool setupGraphics();
bool setupNewCSpace();
bool launchMemoryAllocatorThread();

}