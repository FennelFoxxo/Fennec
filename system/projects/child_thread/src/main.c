#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <string.h>

int initialized = 0;

#define MIN_ALIGN_BYTES 16
#define MIN_ALIGNED __attribute__((aligned (MIN_ALIGN_BYTES)))
static char tls0[4096] MIN_ALIGNED = {};
static char tls1[4096] MIN_ALIGNED = {};

void produce(const char* s);
void consume();

int main(int argc, char *argv[]) {
    printf("Started thread: %s!\n", argv[0]);
    initialized = 1;

    while (1) {
        consume();
    }

    return 0;
}

int main2(int argc, char *argv[]) {
    // Wait for the first thread to initialize musllibc
    while (!initialized);
    
    printf("Started thread: %s\n", argv[0]);

    // Normally sel4runtime would set up the tls, but since main2 is called directly,
    // we need to do it manually
    if (strcmp(argv[0], "producer1_thread") == 0) {
        sel4runtime_move_initial_tls(tls0);
        __sel4_ipc_buffer = (void*)0x528000;
    } else {
        sel4runtime_move_initial_tls(tls1);
        __sel4_ipc_buffer = (void*)0x52b000;
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
