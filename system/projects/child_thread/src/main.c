#include <stdio.h>

int initialized = 0;

int main(int argc, char *argv[]) {
    // Name is first argument
    char* name = argv[0];

    printf("Hello from %s!\n", name);

    initialized = 1;

    while (1);
    return 0;
}

int main2(int argc, char *argv[]) {
    // Wait for the first thread to initialize musllibc
    while (!initialized);

    // Name is first argument
    char* name = argv[0];

    printf("Hello from %s!\n", name);

    while (1);
    return 0;
}