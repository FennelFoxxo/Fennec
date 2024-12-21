#include "runtime_env.hpp"

#include "assert.h"
#include <string.h>

#include <stdio.h>

#define _Static_assert static_assert // Needed for sel4runtime to compile

extern "C" {
#include <sel4runtime.h>
}

void RuntimeEnvironment::setStackTop(uintptr_t stack_top) {
    this->stack_top = stack_top;
}

void RuntimeEnvironment::setProcessName(const char* process_name) {
    this->process_name = process_name;
}

void RuntimeEnvironment::setIPCBufferAddress(uintptr_t ipc_buffer_addr) {
    this->ipc_buffer_addr = ipc_buffer_addr;
    if (ipc_buffer_addr_provided) return;
    ipc_buffer_addr_provided = true;
    num_auxv++;
}

void RuntimeEnvironment::addArg(uintptr_t arg) {
    assert(num_args != RUNTIME_ENVIRONMENT_MAX_ARGS);
    args[num_args++] = arg;
}

uintptr_t RuntimeEnvironment::addCustomData(void* data, int length) {
    assert(custom_data_offset >= length);
    custom_data_offset -= length;
    custom_data_size += length;
    memcpy(custom_data_buffer + custom_data_offset, data, length);
    return stack_top - custom_data_size;
}

int RuntimeEnvironment::write(uintptr_t top) {
    int process_name_size = strlen(process_name) + 1;
    // Arg count + extra arg for process name + empty string + null environment pointer = 4, + null auxv
    int required_stack_structure_length = (4 + num_args) * sizeof(uintptr_t) + (num_auxv + 1) * sizeof(auxv_t);
    int total_length_without_padding = required_stack_structure_length + process_name_size + custom_data_size;
    int padding = (stack_top - total_length_without_padding) % RUNTIME_ENVIRONMENT_STACK_ALIGNMENT;
    int total_length = total_length_without_padding + padding;
    
    uintptr_t bottom = top - total_length;
    uintptr_t ptr = bottom;
    
    printf("Top: 0x%lx, Bottom: 0x%lx\n", top, bottom);
    printf("Total length: %u, padding: %u\n", total_length, padding);
    
    // Write arg count, plus one for the process name
    *(uintptr_t*)ptr = num_args + 1;
    ptr += sizeof(uintptr_t);
    
    // Write string address as first arg, which will go before custom data
    *(uintptr_t*)ptr = stack_top - custom_data_size - process_name_size;
    ptr += sizeof(uintptr_t);
    
    // Write user-provided args
    for (int i = 0; i < num_args; i++) {
        *(uintptr_t*)ptr = args[i];
        ptr += sizeof(uintptr_t);
    }
    
    // Empty string
    *(uintptr_t*)ptr = 0;
    ptr += sizeof(uintptr_t);
    
    // We're not providing any environment pointers yet, so just write null
    *(uintptr_t*)ptr = (uintptr_t)SEL4RUNTIME_NULL;
    ptr += sizeof(uintptr_t);
    
    // Write IPC buffer auxv if provided
    if (ipc_buffer_addr_provided) {
        *(auxv_t*)ptr = auxv_t{.a_type = AT_SEL4_IPC_BUFFER_PTR, .a_un{.a_ptr = (void*)ipc_buffer_addr}};
        ptr += sizeof(auxv_t);
    }
    
    // Write null auxv at the end
    *(auxv_t*)ptr = auxv_t{.a_type = AT_NULL};
    ptr += sizeof(auxv_t);
    
    // Fill in padding
    ptr += padding;
    
    // Write process name
    memcpy((void*)ptr, process_name, process_name_size);
    ptr += process_name_size;
    
    // Write the rest of the custom data
    memcpy((void*)ptr, custom_data_buffer + custom_data_offset, custom_data_size);
    
    return total_length;
}