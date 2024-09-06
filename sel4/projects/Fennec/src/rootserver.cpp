
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
	
	seL4_GetIPCBuffer();


	if (Setup::setup()) printf("Globals setup successfully\n");
	else halt("Failed to setup globals");
	
	if (MemAlloc::start()) printf("Thread started successfully\n");
	else halt("Failed to start thread");
	
	printf("Thing: %p\n", __sel4_ipc_buffer);
	
	seL4_DebugDumpScheduler();

	halt();
    return 0;

while (1);
}