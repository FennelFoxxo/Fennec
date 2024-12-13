#include "graphics.hpp"

#include "globals/globals.h"

#include <utils/util.h>

int getWidth() {
    return Globals::framebuffer_info->width;
}

int getHeight() {
    return Globals::framebuffer_info->height;
}

void setPixel(int x, int y, Color c) {
    int bytes_per_pixel = Globals::framebuffer_info->bpp / 8;
    seL4_Word byte_index = y*Globals::framebuffer_info->pitch + x * bytes_per_pixel;
    unsigned char* addr = (unsigned char*)GRAPHICS_VADDR + byte_index;
    addr[0] = c.b;
    addr[1] = c.g;
    addr[2] = c.r;
}

void rect(int x, int y, int w, int h, Color c) {
    for (int curr_y = y; curr_y < y+h; curr_y++) {
        for (int curr_x = x; curr_x < x+w; curr_x++) {
            setPixel(curr_x, curr_y, c);
        }
    }
}