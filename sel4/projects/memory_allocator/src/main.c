#include "memory_allocator.h"

#include <stdio.h>
#include <sel4/sel4.h>

void foo();

int main(int argc, char* argv[]) {
    seL4_TCB_SetPriority(0, 0, (seL4_Word)argv[1]);
    seL4_TCB_Suspend(0);
}

