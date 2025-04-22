#include <stdio.h>

int main(int argc, char *argv[]) {
    // Name is first argument
    char* name = argv[0];

    printf("Hello from %s!\n", name);

    while (1);
    return 0;
}