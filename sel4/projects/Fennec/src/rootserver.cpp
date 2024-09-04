#include "globals/globals.h"
#include "setup/setup.h"
#include "threads/memalloc.h"


extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
}

void halt(const char* halt_message = nullptr) {
	if (halt_message != nullptr) printf("%s\n", halt_message);
	seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

int main(void)
{	
	

	printf("\n\n--- ROOTSERVER START ---\n\n");

	Globals::Globals g;
	
	if (Setup::setup(g)) printf("Globals setup successfully\n");
	else halt("Failed to setup globals");
	
	if (MemAlloc::start(g)) printf("Thread started successfully\n");
	else halt("Failed to start thread");
	
	seL4_DebugDumpScheduler();

	halt();
    return 0;
	
}