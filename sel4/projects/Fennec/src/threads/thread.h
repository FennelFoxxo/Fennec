#pragma once

#include "globals/globals.h"

namespace Thread {
	
	
struct ThreadConfig {
	// These are relative to root task's cnode
	seL4_CPtr tcb_src_slot = 0; // Which slot to use for storing cap to thread's tcb?
	seL4_CPtr stack_src_slot = 0;
	seL4_CPtr cnode_src_slot = 0;
	seL4_CPtr ipc_src_buffer_slot = 0;
	seL4_CPtr tls_src_slot = 0;
	
	seL4_Word stack_vaddr_offset = 0;
	seL4_Word ipc_vaddr_offset = 0;
	seL4_Word tls_vaddr_offset = 0;
	
	seL4_Uint8 priority = 0;
	
	seL4_Word thread_argument = 0;
	
	void (*thread_func)(seL4_Word arg) = nullptr;
	
	const char* thread_name = nullptr;
};


class Thread {
public:
	Thread(const ThreadConfig& thread_config, bool* success);
	bool start();
	
private:
	bool allocateObjects();
	bool mapObjects();
	bool setupTLS();
	bool copyCSlots();
	bool writeThreadRegisters();
	bool configureThread();
	
	const ThreadConfig config;
	
	
};
	
	
	
}