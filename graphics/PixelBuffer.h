/**
 * @file PixelBuffer.h
 * @brief Declares PixelBuffer — a deferred pixel accumulator for algorithm visualisation.
 *
 * Role in the rendering pipeline
 * ─────────────────────────────────
 * All the custom graphics algorithms (DDA, Bresenham, Midpoint Circle,
 * Scan-Line Fill) write their output into a PixelBuffer rather than
 * directly to the screen.
 *
 * Each frame the Renderer:
 *   1. Calls pixelBuffer.clear().
 *   2. Invokes the selected algorithm (which calls putPixel many times).
 *   3. Calls drawPixelBuffer() to iterate getPixels() and render each
 *      stored pixel as a small coloured dot via ImGui's draw list.
 *
 * This two-phase approach (accumulate then flush) makes it easy to:
 *   - Switch between algorithms without changing the rendering code.
 *   - Visualise algorithm output in "X-Ray mode" as an overlay on the city.
 *
 * Design note
 * ────────────
 * Using a vector of Pixel structs (rather than a flat 2D array) keeps memory
 * proportional to the number of pixels actually drawn, which matters for
 * sparse scenes where only a few lines or circles are being rasterised.
 */

#pragma once

#include <vector>
#include "Color.h"

/**
 * @brief A single coloured pixel at integer screen coordinates.
 *
 * Stored in PixelBuffer; flushed to the ImGui draw list each frame.
 */
struct Pixel {
    int   x;     ///< Horizontal screen coordinate.
    int   y;     ///< Vertical screen coordinate.
    Color color; ///< RGBA colour of this pixel.

    /** @brief Construct a pixel at (x, y) with the given colour. */
    Pixel(int x, int y, Color color)
        : x(x), y(y), color(color) {}
};

/**
 * @brief Accumulates pixels written by rasterization algorithms.
 *
 * Cleared at the beginning of each frame and filled by algorithm calls,
 * then iterated by the Renderer to produce the final pixel-buffer overlay.
 */
class PixelBuffer {
public:
    /** @brief Remove all stored pixels (called at the start of each frame). */
    void clear();

    /**
     * @brief Record a pixel at (x, y) with the given colour.
     *
     * The pixel is appended to the internal vector; it will be drawn
     * during the next drawPixelBuffer() call.
     *
     * @param x      Horizontal screen coordinate.
     * @param y      Vertical screen coordinate.
     * @param color  Pixel colour (RGBA).
     */
    void putPixel(int x, int y, Color color);

    /**
     * @brief Return the list of all accumulated pixels (read-only).
     *
     * Used by the Renderer to iterate and draw each pixel via ImGui.
     */
    const std::vector<Pixel>& getPixels() const;

private:
    std::vector<Pixel> pixels; ///< All pixels accumulated since the last clear().
};