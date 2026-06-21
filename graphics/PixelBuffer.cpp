/**
 * @file PixelBuffer.cpp
 * @brief Implements PixelBuffer — pixel accumulation and retrieval.
 *
 * The implementation is intentionally minimal:
 *  - clear()     → std::vector::clear() — O(n), releases all accumulated pixels.
 *  - putPixel()  → std::vector::emplace_back() — O(1) amortised.
 *  - getPixels() → returns a const reference, no copy made.
 *
 * All actual algorithm logic (which pixels to put) lives in
 * LineAlgorithms, CircleAlgorithms, and FillAlgorithms.
 */
#include "PixelBuffer.h"

void PixelBuffer::clear() {
    // Remove all stored pixels so the next frame starts fresh.
    pixels.clear();
}

void PixelBuffer::putPixel(int x, int y, Color color) {
    // Construct a Pixel in-place at the end of the vector (avoids a copy).
    pixels.emplace_back(x, y, color);
}

const std::vector<Pixel>& PixelBuffer::getPixels() const {
    // Return a const reference so the Renderer can iterate without copying.
    return pixels;
}