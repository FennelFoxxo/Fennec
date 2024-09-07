
#include "globals/globals.h"
#include "setup/setup.h"
#include "threads/memalloc.h"
#include "threads/thread.h"


extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
}

void halt(const char* halt_message = nullptr) {
	if (halt_message != nullptr) printf("%s\n  - Error: %s\n", halt_message, GLOBALS_GET_ERROR());
	seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

bool dothingthatfails() {
	retErrorIfFail(5 < 2, "Five is less than two");
}

int main(void) {	
	printf("\n\n--- ROOTSERVER START ---\n\n");
	
	if (Setup::setup()) printf("Globals setup successfully\n");
	else halt("Failed to setup globals");
	
	bool success;
	Thread::Thread mem_alloc_thread({
		.tcb_src_slot			= Globals::memory_allocator_tcb_slot,
		.stack_src_slot			= Globals::memory_allocator_stack_slot,
		.cnode_src_slot			= Globals::memory_allocator_croot_slot,
		.ipc_src_buffer_slot	= Globals::memory_allocator_ipc_slot,
		.tls_src_slot			= Globals::memory_allocator_tls_slot,
		.stack_vaddr_offset		= MEM_ALLOC_STACK_VADDR,
		.ipc_vaddr_offset		= MEM_ALLOC_IPC_BUFFER_VADDR,
		.tls_vaddr_offset		= MEM_ALLOC_TLS_VADDR,
		.priority				= 250,
		.thread_argument		= 0,
		.thread_func			= MemAlloc::thread,
		.thread_name			= "Memory Allocator"
	}, &success);
	
	if (success) printf("Thread setup successfully\n");
	else halt("Failed to setup thread");
	
	if (mem_alloc_thread.start()) printf("Thread started successfully\n");
	else halt("Failed to start thread");
	
	seL4_DebugDumpScheduler();

	halt();
    return 0;

while (1);
}