#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4runtime.h>

int initialized = 0;
static char thread2_static_tls[4096] __attribute__((aligned (16))) = {};

void produce(seL4_Word data);
void consume();

int main(int argc, char *argv[]) {
    // Name is first argument
    char* name = argv[0];

    printf("Hello from %s!\n", name);

    initialized = 1;

    // Send fibonacci sequence
    int last_num = 0;
    int current_num = 1;
    for (int i = 0; i < 10; i++) {
        produce(current_num);

        int temp_add = last_num + current_num;
        last_num = current_num;
        current_num = temp_add;
    }
    while(1);

    return 0;
}

int main2(int argc, char *argv[]) {
    // Wait for the first thread to initialize musllibc
    while (!initialized);

    // Name is first argument
    char* name = argv[0];

    printf("Hello from %s!\n", name);

    // Normally sel4runtime would set up the tls, but since main2 is called directly,
    // we need to do it manually
    sel4runtime_move_initial_tls(thread2_static_tls);

    while (1) {
        consume();
    }

    return 0;
}

void produce(seL4_Word data) {
    seL4_SetMR(0, data);
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_Send(2, info);
}

void consume() {
    seL4_Word badge;
    seL4_MessageInfo_t response = seL4_Recv(2, &badge);
    seL4_Word data = seL4_GetMR(0);
    printf("Data: %lu, Badge: %lu\n", data, badge);
}