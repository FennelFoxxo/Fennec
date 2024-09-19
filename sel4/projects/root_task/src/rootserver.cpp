
#include "globals/globals.h"
#include "setup/setup.h"
#include "threads/memalloc.h"
#include "threads/thread.h"

#include <memory_allocator/memory_allocator.h>

extern "C" {
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>

#include <elfparser.h>
}


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
	
	printf("Memory allocator elf start: %p, size: %llu\n", memory_allocator_elf_start, memory_allocator_elf_size);
	
	if (Setup::setup()) printf("Globals setup successfully\n");
	else halt("Failed to setup globals");
    
    ElfParser_Header memory_allocator_elf_header;
    ElfParser_Error ep_err = elfparser_get_header(memory_allocator_elf_start, memory_allocator_elf_size, &memory_allocator_elf_header);
    if (ep_err != ELFPARSER_NOERROR) halt("Unable to load memory allocator header");
    
    ElfParser_ProgramHeader program_header;
    for (uint64_t i = 0; i < memory_allocator_elf_header.e_phnum; i++) {
        ep_err = elfparser_get_program_header(memory_allocator_elf_start, &memory_allocator_elf_header, i, &program_header);
        if (ep_err != ELFPARSER_NOERROR) halt("Unable to read program header");
        if (program_header.p_type != ELFPARSER_PT_LOAD) continue;
        printf("  Program Header #%llu vaddr = 0x%llx, size = 0x%llx\n", i, program_header.p_vaddr, program_header.p_memsz);
    }
    
    
    
    
	
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