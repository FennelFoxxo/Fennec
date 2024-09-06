#include "memalloc.h"

#include "threads/memalloc.h"

extern "C" {
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <stdio.h>
}

bool configureThread() {
	seL4_DebugNameThread(Globals::memory_allocator_tcb_slot, "Memory Allocator");
	
	seL4_Error error = seL4_TCB_Configure(	Globals::memory_allocator_tcb_slot, seL4_CapNull,
											Globals::memory_allocator_croot_slot, 0,								// CSpace args
											seL4_CapInitThreadVSpace, 0,												// VSpace args
											(seL4_Word)MEM_ALLOC_IPC_BUFFER_VADDR, Globals::memory_allocator_ipc_buffer_slot);	// IPC args
	retFalseIfFail(error == seL4_NoError);

	error = seL4_TCB_SetPriority(Globals::memory_allocator_tcb_slot, seL4_CapInitThreadTCB, MEM_ALLOC_PRIORITY);
	retFalseIfFail(error == seL4_NoError);
	
	return true;
}

bool writeThreadRegisters(seL4_Word arg) {
	seL4_Error error;
	
	seL4_UserContext regs;
	error = seL4_TCB_ReadRegisters(Globals::memory_allocator_tcb_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	if (error != seL4_NoError) return false;

	regs.rip = (seL4_Word)&MemAlloc::thread;
	regs.rsp = (seL4_Word)&Globals::memory_allocator_stack;
	regs.rdi = arg;

	error = seL4_TCB_WriteRegisters(Globals::memory_allocator_tcb_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	if (error != seL4_NoError) return false;
	
	return true;
}

bool copyCSlots() {
	seL4_Error error;
	
	// Setup root cap in target thread cspace
	error = seL4_CNode_Copy(Globals::memory_allocator_croot_slot, MEM_ALLOC_SLOT_CNODE, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, Globals::memory_allocator_tcb_slot, seL4_WordBits, seL4_AllRights);
	if (error != seL4_NoError) return false;
	
	// Copy L2 memory CNode into target thread cspace
	error = seL4_CNode_Copy(Globals::memory_allocator_croot_slot, MEM_ALLOC_SLOT_L2_MEM_CNODE, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, Globals::L2_memory_slot, seL4_WordBits, seL4_AllRights);
	if (error != seL4_NoError) return false;

	return true;
}

bool allocateObjects() {
	// Create thread TCB
	seL4_Error error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_TCBObject, seL4_TCBBits, seL4_CapInitThreadCNode, 0, 0, Globals::memory_allocator_tcb_slot, 1);
	retFalseIfFail(error == seL4_NoError);

	// Create root cnode, sized to fill a page
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_CapTableObject, GLOBALS_CNODE_BITS, seL4_CapInitThreadCNode, 0, 0, Globals::memory_allocator_croot_slot, 1);
	retFalseIfFail(error == seL4_NoError);
	
	// Create IPC buffer
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, Globals::memory_allocator_ipc_buffer_slot, 1);
	retFalseIfFail(error == seL4_NoError);
	
	// Create TLS
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, Globals::memory_allocator_tls_slot, 1);
	retFalseIfFail(error == seL4_NoError);

	return true;
}

bool mapObjects() {
	seL4_Error error = seL4_X86_Page_Map(Globals::memory_allocator_ipc_buffer_slot, seL4_CapInitThreadVSpace, MEM_ALLOC_IPC_BUFFER_VADDR, seL4_ReadWrite, seL4_X86_Default_VMAttributes);
	retFalseIfFail(error == seL4_NoError);
	
	error = seL4_X86_Page_Map(Globals::memory_allocator_tls_slot, seL4_CapInitThreadVSpace, MEM_ALLOC_TLS_VADDR, seL4_ReadWrite, seL4_X86_Default_VMAttributes);
	retFalseIfFail(error == seL4_NoError);
	
	return true;
}

bool setupTLS() {
	seL4_Word tls = sel4runtime_write_tls_image((void*)MEM_ALLOC_TLS_VADDR);
	printf("Start\n");
	int error = sel4runtime_set_tls_variable(tls, __sel4_ipc_buffer, (seL4_IPCBuffer*)MEM_ALLOC_IPC_BUFFER_VADDR);
	printf("Error: %d\n", error);
	retFalseIfFail(error == 0);
	
	error = seL4_TCB_SetTLSBase(Globals::memory_allocator_tcb_slot, tls);
	printf("Error 2: %d\n", error);
	retFalseIfFail(error == seL4_NoError);
	
	return true;
}

bool Setup::setupMemAllocThread() {
	retFalseIfFail(allocateObjects());
	retFalseIfFail(mapObjects());
	retFalseIfFail(setupTLS());
	retFalseIfFail(copyCSlots());
	retFalseIfFail(configureThread());
	retFalseIfFail(writeThreadRegisters((seL4_Word)(12345)));

	return true;
}