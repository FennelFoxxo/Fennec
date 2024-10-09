#include "memory_allocator.h"

#include <stdio.h>
#include <sel4/sel4.h>

int main(seL4_Word arg) {
    seL4_TCB_SetPriority(0, 0, arg);
    seL4_TCB_Suspend(0);
}

