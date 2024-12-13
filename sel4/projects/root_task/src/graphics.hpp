#pragma once

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

int getWidth();
int getHeight();
void setPixel(int x, int y, Color c);
void rect(int x, int y, int w, int h, Color c);