#pragma once

#include <vector>
#include "Color.h"

struct Pixel {
    int x;
    int y;
    Color color;

    Pixel(int x, int y, Color color)
        : x(x), y(y), color(color) {}
};

class PixelBuffer {
public:
    void clear();
    void putPixel(int x, int y, Color color);
    const std::vector<Pixel>& getPixels() const;

private:
    std::vector<Pixel> pixels;
};