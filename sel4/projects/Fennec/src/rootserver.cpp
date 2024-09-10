
#include "globals/globals.h"
#include "setup/setup.h"
#include "threads/memalloc.h"
#include "threads/thread.h"

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
}


extern void* _binary_memory_allocator_exec_start;
extern void* _binary_memory_allocator_exec_size;

void* memory_allocator_elf_start = &_binary_memory_allocator_exec_start;
seL4_Word memory_allocator_elf_size = (seL4_Word)&_binary_memory_allocator_exec_size;



void halt(const char* halt_message = nullptr) {
	if (halt_message != nullptr) printf("%s\n  - Error: %s\n", halt_message, GLOBALS_GET_ERROR());
	seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

void dumpelf() {
	printf("Dumping contents of elf file:\n");
	for (seL4_Word i = 0; i < memory_allocator_elf_size; i++) {
		if (i % 16 == 0) {
			printf("\n%.4lx - ", i);
		}
		printf("%.2x ", ((unsigned char*)memory_allocator_elf_start)[i]);
	}
	printf("\n\n");
}

int main(void) {
	printf("\n\n--- ROOTSERVER START ---\n\n");
	
	dumpelf();
	
	printf("Memory allocator elf start: %p, size: %lu\n", memory_allocator_elf_start, memory_allocator_elf_size);
	
	if (Setup::setup()) printf("Globals setup successfully\n");
	else halt("Failed to setup globals");
	
	bool success;
	Thread::Thread mem_alloc_thread({
		.tcb_src_slot			= Globals::memory_allocator_tcb_slot,
		.stack_src_slot			= Globals::memory_allocator_stack_slot,
		.cnode_src_slot			= Globals::memory_allocator_croot_slot,
		.ipc_src_buffer_slot	= Globals::memory_allocator_ipc_slot,
		.tls_src_slot			= Globals::memory_allocator_tls_slot,
		
		.extra_slots = {Globals::L2_memory_slot},
		.num_extra_slots = 1,
		
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
}