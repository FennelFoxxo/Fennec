#include "globals/globals.h"
#include "setup/setup.h"

#include "stack.hpp"
#include "graphics.hpp"
#include "mapping.hpp"

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
    
    if (Globals::framebuffer_info) {
        printf("addr: %lx\n", Globals::framebuffer_info->addr);
        printf("pitch: %u\n", Globals::framebuffer_info->pitch);
        printf("width: %u\n", Globals::framebuffer_info->width);
        printf("height: %u\n", Globals::framebuffer_info->height);
        printf("bpp: %u\n", Globals::framebuffer_info->bpp);
        printf("type: %u\n", Globals::framebuffer_info->type);
    }
    
    if (Setup::breakMemoryIntoChunks()) printf("Chunked memory successfully\n");
	else halt("Failed to create chunked memory");
    
    rect(0, 0, getWidth(), 25, {0, 255, 0});
    rect(0, 25, getWidth(), getHeight()-25, {50, 50, 50});
    
    
    
    if (Setup::launchMemoryAllocatorThread()) printf("Launched memory allocator successfully\n");
	else halt("Failed to launch memory allocator");
    
    rect(0, 25, getWidth(), 25, {0, 128, 0});
    
    seL4_Yield();
    
	seL4_DebugDumpScheduler();
    

	halt();
    return 0;
}