#pragma once

#include "globals/globals.h"

namespace Setup {

void initBootInfo();
void breakMemoryIntoChunks();
void setupMapping();
void setupGraphics();
void setupNewCSpace();
void launchMemoryAllocatorThread();

}