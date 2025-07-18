#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <string.h>
#include <stdlib.h>
#include <tailspring.h>

int initialized = 0;

#define MIN_ALIGN_BYTES 16
#define MIN_ALIGNED __attribute__((aligned (MIN_ALIGN_BYTES)))

void produce(const char* s);
void consume();

int startswith(const char *str, const char *prefix) {
    return strncmp(prefix, str, strlen(prefix)) == 0;
}


int main(int argc, char *argv[], char *envp[]) {
    printf("Started thread: %s!\n", argv[0]);
    printf("  Envp: ");
    for (char** s = envp; *s; s++) {
        printf("%s ", *s);
    }
    printf("\n");
    printf("tailspring_lib_test_int: %d\n", tailspring_lib_test_int);
    initialized = 1;

    while (1) {
        consume();
    }

    return 0;
}

int main2(int argc, char *argv[], char *envp[]) {
    // Wait for the first thread to initialize musllibc
    while (!initialized);

    printf("Started thread: %s\n", argv[0]);

    printf("  Args: ");
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    char tls[4096] MIN_ALIGNED = {};
    sel4runtime_move_initial_tls(tls);

    // Environment pointers
    for (char** env_ptr = envp; *env_ptr; env_ptr++) {
        char* env_var = *env_ptr;
        // Look for ipc buffer environment pointer
        if (startswith(env_var, "ipc_buffer=")) {
            // Convert to int
            char* ipc_buffer_addr_str = strchr(env_var, '=') + 1;
            long long unsigned ipc_buffer_addr = atoll(ipc_buffer_addr_str);

            // Set ipc buffer
            __sel4_ipc_buffer = (void*)ipc_buffer_addr;

        }

        if (startswith(env_var, "gp_memory_info=")) {
            // Convert to int
            char* gp_memory_info_addr_str = strchr(env_var, '=') + 1;
            long long unsigned gp_memory_info_addr = atoll(gp_memory_info_addr_str);

            printf("Received gp memory info at addr %llx\n", gp_memory_info_addr);

            seL4_Word num_untypeds = *(seL4_Word*)gp_memory_info_addr;
            printf("Num untypeds: %lu\n", num_untypeds);
            for (int i = 0; i < num_untypeds; i++) {
                seL4_Word size_bits = ((seL4_Word*)gp_memory_info_addr)[i+1];
                printf("Untyped %d size bits: %lu\n", i, size_bits);

            }
        }
    }

    char buffer[100];
    snprintf(buffer, 100, "Hello from thread %s!", argv[0]);

    // Send name
    produce(buffer);
    while(1);

    return 0;
}

void produce(const char* s) {
    seL4_Word len = strlen(s) + 1;
    for (seL4_Word i = 0; i < len; i++) {
        seL4_SetMR(i, s[i]);
    }
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, len);
    seL4_Send(1, info);
}

void consume() {
    seL4_Word badge;
    seL4_MessageInfo_t response = seL4_Recv(1, &badge);
    seL4_Word len = seL4_MessageInfo_get_length(response);
    printf("Badge: %lu, Length: %lu, Data: ", badge, len);
    for (seL4_Word i = 0; i < len; i++) {
        printf("%c", (char)seL4_GetMR(i));
    }
    printf("\n");
}
