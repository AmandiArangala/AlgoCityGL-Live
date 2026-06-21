/**
 * @file PixelBuffer.h
 * @brief Declares PixelBuffer — a deferred pixel accumulator for algorithm visualisation.
 *
 * Role in the rendering pipeline:
 *  - Classic rasterisation algorithms (DDA, Bresenham, Midpoint Circle, Scan-Line Fill)
 *    write pixels into this buffer instead of directly to the screen.
 *  - After each frame, Renderer::drawPixelBuffer() reads the accumulated pixels
 *    and draws them as coloured dots using Dear ImGui's AddCircleFilled.
 *  - The buffer is cleared at the start of each scene build so stale pixels
 *    from the previous frame do not persist.
 */
#pragma once

#include <vector>
#include "Color.h"

/**
 * @brief A single rasterised pixel with a screen position and colour.
 */
struct Pixel {
    int   x;      ///< Screen x coordinate (pixels from left).
    int   y;      ///< Screen y coordinate (pixels from top).
    Color color;  ///< RGBA colour of this pixel.

    /// Constructs a pixel at (x, y) with the given colour.
    Pixel(int x, int y, Color color)
        : x(x), y(y), color(color) {}
};

/**
 * @brief A growable list of Pixel values produced by rasterisation algorithms.
 *
 * Algorithms call putPixel() to accumulate pixels, and the Renderer calls
 * getPixels() to draw them.  Calling clear() resets the buffer for the
 * next frame.
 */
class PixelBuffer {
public:
    /** @brief Removes all pixels from the buffer (O(n)). */
    void clear();

    /** @brief Appends a single pixel at screen position (x, y) with the given colour. */
    void putPixel(int x, int y, Color color);

    /** @brief Returns a const reference to the internal pixel list for rendering. */
    const std::vector<Pixel>& getPixels() const;

private:
    std::vector<Pixel> pixels; ///< Contiguous storage of accumulated pixels.
};