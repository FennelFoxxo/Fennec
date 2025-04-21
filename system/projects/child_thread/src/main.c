int flag = 0;

void _start() {
    // Segfault at dummy address to test that this program is being loaded correctly

    if (flag) {
        *(int*)0x123456 = 0;
    } else {
        flag = 1;
        *(int*)0x987654 = 0;
    }
    while (1);
}