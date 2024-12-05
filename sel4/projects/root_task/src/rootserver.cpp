#include "globals/globals.h"
#include "setup/setup.h"

#include <stack.hpp>

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
}

void halt(const char* halt_message = nullptr) {
	if (halt_message != nullptr) printf("%s\n  - Error: %s\n", halt_message, GLOBALS_GET_ERROR());
	seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}


int main(void) {
	printf("\n\n--- ROOTSERVER START ---\n\n");
    
    if (Setup::initBootInfo()) printf("Global info setup successfully\n");
	else halt("Failed to setup global info");
    
    if (Setup::breakMemoryIntoChunks()) printf("Chunked memory successfully\n");
	else halt("Failed to create chunked memory");
    
    if (Setup::reserveRequiredMemory()) printf("Reserved required memory successfully\n");
	else halt("Failed to reserve required memory");
    
    if (Setup::setupNewCSpace()) printf("Setup new cspace successfully\n");
	else halt("Failed to setup new cspace");
    
    if (Setup::setupBootstrapMapping()) printf("Bootstrap mapping setup successfully\n");
	else halt("Failed to setup bootstrap mapping");
    
    if (Setup::launchMemoryAllocatorThread()) printf("Launched memory allocator successfully\n");
	else halt("Failed to launch memory allocator");
    
    seL4_Yield();
    
	seL4_DebugDumpScheduler();
    
	halt();
    return 0;
}