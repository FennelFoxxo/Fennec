#include "globals.h"

namespace Globals {

seL4_BootInfo* boot_info = nullptr;

const char* error_msg;

seL4_Word num_empty_slots = 0;
seL4_Word num_memory_chunks = 0;

seL4_Word memory_chunks_allocable_start = 0;

seL4_CPtr bootstrap_empty_start = 0;

char memory_allocator_stack[1024] __attribute__((aligned(16)));

}