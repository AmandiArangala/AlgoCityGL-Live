#include "PixelBuffer.h"

void PixelBuffer::clear() {
    pixels.clear();
}

void PixelBuffer::putPixel(int x, int y, Color color) {
    pixels.emplace_back(x, y, color);
}

const std::vector<Pixel>& PixelBuffer::getPixels() const {
    return pixels;
}