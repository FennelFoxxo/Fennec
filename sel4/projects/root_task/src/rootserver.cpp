#include "globals/globals.h"
#include "setup/setup.h"

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
    
    seL4_Word badge;
    seL4_MessageInfo_t response = seL4_Recv(GLOBALS_ASSORTED_CSLOT(memory_allocator_endpoint), &badge);
    
    for (seL4_Word i = 0; i < seL4_MessageInfo_get_length(response); i++) {
        char c = (char)seL4_GetMR(i);
        printf("%c", c);
    }
    printf("\n");

	halt();
    return 0;
}