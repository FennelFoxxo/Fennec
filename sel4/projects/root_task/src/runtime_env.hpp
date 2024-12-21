#pragma once

#include <stdint.h>

#define RUNTIME_ENVIRONMENT_MAX_ARGS 32
#define RUNTIME_ENVIRONMENT_CUSTOM_DATA_BUFFER_SIZE 512
#define RUNTIME_ENVIRONMENT_STACK_ALIGNMENT 16

// For a program that has been linked with sel4runtime, the program expects
// the stack to be set up in a specific way. This class builds up the stack
// and then writes it to a specific address, which can be mapped into the
// destination thread's address space. Custom data may be placed above the stack
class RuntimeEnvironment {
public:
    RuntimeEnvironment() {}
    
    // Sets the location of where the top of the stack will be placed
    // Affects any addresses returned by other methods, as these
    // addresses will be relative to the stack top
    void setStackTop(uintptr_t stack_top);
    
    // Sets the name of the process, which will be implicitly passed as the
    // first argument to the thread
    void setProcessName(const char* process_name);
    
    // Set IPC buffer address which is used by the runtime to set up IPC
    void setIPCBufferAddress(uintptr_t ipc_buffer_addr);
    
    // Adds an argument to be passed to the thread
    void addArg(uintptr_t arg);
    
    // Places some custom data above the required stack structure
    // Can be called multiple times
    // Returns the vaddr of the data, based on the top of the stack
    uintptr_t addCustomData(void* data, int length);
    
    // Writes the stack, along with the custom data, to the given address
    // The address is where the TOP of the stack should be, the actual stack
    // will be written below and up to this address!
    // Returns the number of bytes written
    int write(uintptr_t top);
    
private:
    const char* process_name = "";
    
    uintptr_t stack_top = 0;
    uintptr_t ipc_buffer_addr = 0;
    bool ipc_buffer_addr_provided = false;
    int num_args = 0;
    int num_auxv = 0;
    
    // Since the stack builds down, we'll do the same for our custom data
    int custom_data_offset = RUNTIME_ENVIRONMENT_CUSTOM_DATA_BUFFER_SIZE;
    int custom_data_size = 0;
    
    uintptr_t args[RUNTIME_ENVIRONMENT_MAX_ARGS];
    uint8_t custom_data_buffer[RUNTIME_ENVIRONMENT_CUSTOM_DATA_BUFFER_SIZE];
};