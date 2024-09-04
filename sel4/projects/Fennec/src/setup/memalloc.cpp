#include "memalloc.h"

#include "threads/memalloc.h"


extern "C" {
#include <sel4/sel4.h>
}

// Globals are used everywhere in here and it's annoying to pass them to every function, so this is set in setupMemAllocThread and used as a shorthand in all the other functions
static Globals::Globals* globals_ptr; 

bool configureThread() {
	seL4_DebugNameThread(globals_ptr->memory_allocator_tcb_slot, "Memory Allocator");
	
	seL4_Error error = seL4_TCB_Configure(	globals_ptr->memory_allocator_tcb_slot, seL4_CapNull,
											globals_ptr->memory_allocator_croot_slot, 0,								// CSpace args
											seL4_CapInitThreadVSpace, 0,												// VSpace args
											(seL4_Word)globals_ptr->boot_info->ipcBuffer, seL4_CapInitThreadIPCBuffer);	// IPC args
	retFalseIfFail(error == seL4_NoError);

	error = seL4_TCB_SetPriority(globals_ptr->memory_allocator_tcb_slot, seL4_CapInitThreadTCB, MEM_ALLOC_PRIORITY);
	retFalseIfFail(error == seL4_NoError);
	
	return true;
}

bool writeThreadRegisters(seL4_Word arg) {
	seL4_Error error;
	
	seL4_UserContext regs;
	error = seL4_TCB_ReadRegisters(globals_ptr->memory_allocator_tcb_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	if (error != seL4_NoError) return false;

	regs.rip = (seL4_Word)&MemAlloc::thread;
	regs.rsp = (seL4_Word)&globals_ptr->memory_allocator_stack;
	regs.rdi = arg;

	error = seL4_TCB_WriteRegisters(globals_ptr->memory_allocator_tcb_slot, 0, 0, sizeof(regs)/sizeof(seL4_Word), &regs);
	if (error != seL4_NoError) return false;
	
	return true;
}

bool copyCSlots() {
	seL4_Error error;
	
	// Setup root cap in target thread cspace
	error = seL4_CNode_Copy(globals_ptr->memory_allocator_croot_slot, MEM_ALLOC_SLOT_CNODE, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, globals_ptr->memory_allocator_tcb_slot, seL4_WordBits, seL4_AllRights);
	if (error != seL4_NoError) return false;
	
	// Copy L2 memory CNode into target thread cspace
	error = seL4_CNode_Copy(globals_ptr->memory_allocator_croot_slot, MEM_ALLOC_SLOT_L2_MEM_CNODE, GLOBALS_CNODE_BITS,
							seL4_CapInitThreadCNode, globals_ptr->L2_memory_slot, seL4_WordBits, seL4_AllRights);
	if (error != seL4_NoError) return false;

	return true;
}

bool allocateObjects() {
	seL4_Error error;
	
	// Create thread TCB
	error = seL4_Untyped_Retype(globals_ptr->bootstrap_memory_slot, seL4_TCBObject, seL4_TCBBits, seL4_CapInitThreadCNode, 0, 0, globals_ptr->memory_allocator_tcb_slot, 1);
	if (error != seL4_NoError) return false;

	// Create root cnode, sized to fill a page
	error = seL4_Untyped_Retype(globals_ptr->bootstrap_memory_slot, seL4_CapTableObject, GLOBALS_CNODE_BITS, seL4_CapInitThreadCNode, 0, 0, globals_ptr->memory_allocator_croot_slot, 1);
	if (error != seL4_NoError) return false;

	return true;
}

bool Setup::setupMemAllocThread(Globals::Globals& globals) {
	globals_ptr = &globals;
	if (!allocateObjects()) return false;
	if (!copyCSlots()) return false;
	if (!configureThread()) return false;
	if (!writeThreadRegisters((seL4_Word)(12345))) return false;

	return true;
}