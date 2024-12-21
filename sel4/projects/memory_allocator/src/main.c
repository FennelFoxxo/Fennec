#include "memory_allocator.h"

#include <stdio.h>
#include <sel4/sel4.h>

void foo();

int main(int argc, char* argv[]) {
    int* arr = (int*)argv[2];
    int* arr2 = (int*)argv[3];
    seL4_TCB_SetPriority(0, 0, (seL4_Word)(arr[3] + arr2[1]));
    seL4_TCB_Suspend(0);
}

