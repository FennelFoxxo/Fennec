#include "globals.h"

namespace Globals {

seL4_BootInfo* boot_info = nullptr;

seL4_Word num_empty_slots = 0;
seL4_Word num_memory_chunks = 0;

seL4_Word memory_chunks_next_available = 0;

bool has_memory_tree_been_moved = false;

MappingContext mapping_context;

seL4_CPtr bootstrap_empty_start = 0;

MultibootFrameBuffer* framebuffer_info = nullptr;

char memory_allocator_stack[1024] __attribute__((aligned(16)));

}