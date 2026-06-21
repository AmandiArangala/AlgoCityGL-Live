/**
 * @file PixelBuffer.cpp
 * @brief Implements PixelBuffer — pixel accumulation and retrieval.
 *
 * The implementation is intentionally minimal:
 *  - clear()     → std::vector::clear() — O(n), releases all pixels.
 *  - putPixel()  → std::vector::emplace_back() — amortized O(1) append.
 *  - getPixels() → returns a const reference; no copying.
 *
 * Performance note
 * ─────────────────
 * For each frame the vector is cleared and re-populated from scratch.
 * Clearing a vector does NOT free its heap allocation — the internal
 * capacity stays reserved, so subsequent pushes don't trigger reallocation
 * after the first few frames.  This makes the per-frame cost very low.
 */

#include "PixelBuffer.h"

/**
 * @brief Remove all stored pixels.
 *
 * Resets the size to 0 while keeping the allocated capacity intact,
 * so the next frame's putPixel() calls are likely allocation-free.
 */
void PixelBuffer::clear() {
    pixels.clear();
}

/**
 * @brief Append a new pixel at screen position (x, y) with the given colour.
 *
 * Uses emplace_back to construct the Pixel in-place within the vector,
 * avoiding an extra copy compared to push_back(Pixel(...)).
 */
void PixelBuffer::putPixel(int x, int y, Color color) {
    pixels.emplace_back(x, y, color);
}

/**
 * @brief Return a read-only reference to all accumulated pixels.
 *
 * The Renderer iterates this vector to draw each pixel as an ImGui point.
 * The reference is valid until the next clear() call.
 */
const std::vector<Pixel>& PixelBuffer::getPixels() const {
    return pixels;
}