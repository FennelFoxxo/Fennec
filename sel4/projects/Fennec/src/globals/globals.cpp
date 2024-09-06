#include "globals.h"

namespace Globals {

seL4_BootInfo* boot_info = nullptr;

seL4_Word num_empty_slots = 0;
seL4_Word num_memory_chunks = 0;

seL4_Word memory_chunks_allocable_start = 0;

seL4_CPtr bootstrap_memory_slot = 0;
seL4_CPtr L2_memory_slot = 0;
seL4_CPtr page_directory_slot = 0;
seL4_CPtr page_table_slot = 0;

// Memory allocator

seL4_CPtr memory_allocator_tcb_slot = 0;
seL4_CPtr memory_allocator_croot_slot = 0;
seL4_CPtr memory_allocator_ipc_buffer_slot = 0;
seL4_CPtr memory_allocator_tls_slot = 0;

char memory_allocator_stack[1024] __attribute__((aligned(16)));

}