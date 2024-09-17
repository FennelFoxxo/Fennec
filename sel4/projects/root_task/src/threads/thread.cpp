#include "thread.h"

extern "C" {
#include <sel4runtime.h>
#include <stdio.h>
}

namespace Thread {

Thread::Thread(const ThreadConfig& thread_config, bool* success) : config(thread_config) {
	*success = false;

	retIfFail(allocateObjects());
	retIfFail(mapObjects());
	retIfFail(setupTLS());
	retIfFail(copyCSlots());
	retIfFail(configureThread());
	retIfFail(writeThreadRegisters());

	*success = true;
}

bool Thread::start() {
	seL4_Error error = seL4_TCB_Resume(config.tcb_src_slot);
	return error == seL4_NoError;
}


bool Thread::allocateObjects() {
	// Create thread TCB
	seL4_Error error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_TCBObject, seL4_TCBBits, seL4_CapInitThreadCNode, 0, 0, config.tcb_src_slot, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into TCB during thread setup");
	
	// Create stack, sized to fill a page
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, config.stack_src_slot, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into stack frame during thread setup");

	// Create root cnode, sized to fill a page
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_CapTableObject, GLOBALS_CNODE_BITS, seL4_CapInitThreadCNode, 0, 0, config.cnode_src_slot, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into cnode during thread setup");
	
	// Create IPC buffer
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, config.ipc_src_buffer_slot, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into IPC frame during thread setup");
	
	// Create TLS
	error = seL4_Untyped_Retype(Globals::bootstrap_memory_slot, seL4_X86_4K, 0, seL4_CapInitThreadCNode, 0, 0, config.tls_src_slot, 1);
	retErrorIfFail(error == seL4_NoError, "Failed to retype into TLS frame during thread setup");

	return true;
}


bool Thread::mapObjects() {
	// Map stack
	seL4_Error error = seL4_X86_Page_Map(	config.stack_src_slot, seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR + config.stack_vaddr_offset,
											seL4_ReadWrite, seL4_X86_Default_VMAttributes); // Attributes
	retErrorIfFail(error == seL4_NoError, "Failed to map stack during thread setup");
	
	
	// Map IPC buffer
	error = seL4_X86_Page_Map(	config.ipc_src_buffer_slot, seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR + config.ipc_vaddr_offset,
								seL4_ReadWrite, seL4_X86_Default_VMAttributes); // Attributes
	retErrorIfFail(error == seL4_NoError, "Failed to map IPC buffer during thread setup");
	
	// Map TLS
	error = seL4_X86_Page_Map(	config.tls_src_slot, seL4_CapInitThreadVSpace, BOOTSTRAP_VADDR + config.tls_vaddr_offset,
								seL4_ReadWrite, seL4_X86_Default_VMAttributes); // Attributes
	retErrorIfFail(error == seL4_NoError, "Failed to map TLS during thread setup");
	
	return true;
}

bool Thread::setupTLS() {
	seL4_Word tls = sel4runtime_write_tls_image((void*)(BOOTSTRAP_VADDR + config.tls_vaddr_offset));

	int error = sel4runtime_set_tls_variable(tls, __sel4_ipc_buffer, (seL4_IPCBuffer*)(BOOTSTRAP_VADDR + config.ipc_vaddr_offset));
	retErrorIfFail(error == seL4_NoError, "Failed to set TLS variable during thread setup");
	
	error = seL4_TCB_SetTLSBase(config.tcb_src_slot, tls);
	retErrorIfFail(error == seL4_NoError, "Failed to set TLS base during thread setup");
	
	return true;
}


bool Thread::copyCSlots() {
	// Setup root cap in target thread cspace
	seL4_Error error = seL4_CNode_Copy(	config.cnode_src_slot, 1, GLOBALS_CNODE_BITS,
										seL4_CapInitThreadCNode, config.cnode_src_slot, seL4_WordBits, seL4_AllRights);
	retErrorIfFail(error == seL4_NoError, "Failed to copy thread cspace cap into thread cspace during thread setup");
	
	// Setup TCB in target thread cspace
	error = seL4_CNode_Copy(config.cnode_src_slot, 2, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, config.tcb_src_slot, seL4_WordBits, seL4_AllRights);
	retErrorIfFail(error == seL4_NoError, "Failed to copy thread TCB cap into thread cspace during thread setup");
	
	// Copy extra slots into thread cspace
	for (seL4_Word i = 0; i < config.num_extra_slots; i++) {
		error = seL4_CNode_Copy(Globals::memory_allocator_croot_slot, THREAD_EXTRA_SLOTS + i, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, config.extra_slots[i], seL4_WordBits, seL4_AllRights);
		retErrorIfFail(error == seL4_NoError, "Failed to copy extra slots into thread cspace during thread setup");
	}

	return true;
}


bool Thread::writeThreadRegisters() {
	seL4_UserContext regs;
	seL4_Error error = seL4_TCB_ReadRegisters(config.tcb_src_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to read registers during thread setup");

	regs.rip = (seL4_Word)config.thread_func;
	regs.rsp = BOOTSTRAP_VADDR + config.stack_vaddr_offset + 0x1000; // Set stack pointer to top of stack
	regs.rdi = config.thread_argument;

	error = seL4_TCB_WriteRegisters(config.tcb_src_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	retErrorIfFail(error == seL4_NoError, "Failed to write registers during thread setup");
	
	return true;
}


bool Thread::configureThread() {
	seL4_DebugNameThread(config.tcb_src_slot, config.thread_name);
	
	seL4_Error error = seL4_TCB_Configure(	config.tcb_src_slot, seL4_CapNull,
											config.cnode_src_slot, 0,												// CSpace args
											seL4_CapInitThreadVSpace, 0,											// VSpace args
											BOOTSTRAP_VADDR + config.ipc_vaddr_offset, config.ipc_src_buffer_slot);	// IPC args
	retErrorIfFail(error == seL4_NoError, "Failed to configure thread during thread setup");

	error = seL4_TCB_SetPriority(config.tcb_src_slot, seL4_CapInitThreadTCB, config.priority);
	retErrorIfFail(error == seL4_NoError, "Failed to set priority during thread setup");
	
	return true;
}


}