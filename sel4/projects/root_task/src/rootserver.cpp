#include "globals/globals.h"
#include "setup/setup.h"

#include "stack.hpp"
#include "graphics.hpp"
#include "mapping.hpp"
#include "assert.h"

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
}

void halt(const char* err = nullptr) {
	if (err) printf("%s\n", err);
	seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

int main(void) {
    setAssertFailCallback(&halt);
    
	printf("\n\n--- ROOTSERVER START ---\n\n");
    
    Setup::initBootInfo();
    printf("Global info setup successfully\n");
    
    Setup::breakMemoryIntoChunks();
    printf("Chunked memory successfully\n");
    
    rect(0, 0, getWidth(), 25, {0, 255, 0});
    rect(0, 25, getWidth(), getHeight()-25, {50, 50, 50});
    
    Setup::launchMemoryAllocatorThread();
    printf("Launched memory allocator successfully\n");
    
    rect(0, 25, getWidth(), 25, {0, 128, 0});
    
    seL4_Yield();
    
	seL4_DebugDumpScheduler();
    

	halt();
    return 0;
}