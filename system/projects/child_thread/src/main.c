void _start() {
    // Segfault at dummy address to test that this program is being loaded correctly
    *(char*)0xf00f00baba = 0;
}