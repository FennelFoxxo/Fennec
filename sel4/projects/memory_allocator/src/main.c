#include "memory_allocator.h"

#include <stdio.h>
#include <sel4/sel4.h>

void foo();

int main(int argc, char* argv[]) {
    int* arr = (int*)argv[2];
    int* arr2 = (int*)argv[3];

    char* message = "Hello from memory allocator thread!\n  Yippee!";

    int length;
    for (length = 0; message[length]; length++) {
        seL4_SetMR(length, message[length]);
    }

    seL4_MessageInfo_t info = seL4_MessageInfo_new(52, 0, 0, length);

    seL4_Send(1, info);

    seL4_TCB_SetPriority(0, 0, (seL4_Word)(arr[3] + arr2[1]));
    seL4_TCB_Suspend(0);
}

