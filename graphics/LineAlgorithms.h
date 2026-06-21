/**
 * @file LineAlgorithms.h
 * @brief Declares two classic rasterization algorithms for drawing lines into a PixelBuffer.
 *
 * Both algorithms convert a mathematical line segment defined by two endpoints
 * into a discrete set of pixels.  The user can switch between them at runtime
 * using the ImGui panel.
 *
 * DDA (Digital Differential Analyzer)
 * ─────────────────────────────────────
 * Computes the larger delta (dx or dy) as the number of steps, then advances
 * x and y by equal floating-point increments each step.  Simple to understand
 * but performs floating-point arithmetic every pixel.
 *
 * Bresenham's Line Algorithm
 * ──────────────────────────
 * Uses only integer arithmetic (addition and bit-shifts) to decide whether to
 * step horizontally, vertically, or diagonally.  This makes it faster and
 * avoids accumulated floating-point rounding errors.
 *
 * Both static methods write directly into a PixelBuffer via PixelBuffer::putPixel().
 */

#pragma once

#include "PixelBuffer.h"

class LineAlgorithms {
public:
    /**
     * @brief Draw a line using the DDA (Digital Differential Analyzer) algorithm.
     *
     * Algorithm:
     *  1. Compute dx = x2-x1 and dy = y2-y1.
     *  2. steps = max(|dx|, |dy|)  — the dominant axis determines the step count.
     *  3. xIncrement = dx / steps,  yIncrement = dy / steps.
     *  4. Iterate `steps` times, rounding (x, y) to the nearest pixel each step.
     *
     * @param buffer  Pixel buffer to draw into.
     * @param x1, y1  Start endpoint (integer screen coordinates).
     * @param x2, y2  End endpoint (integer screen coordinates).
     * @param color   Pixel colour to write.
     */
    static void drawLineDDA(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );

    /**
     * @brief Draw a line using Bresenham's integer line algorithm.
     *
     * Algorithm:
     *  1. Compute |dx| and |dy|; determine step directions sx, sy.
     *  2. Maintain an integer `error` term (initially dx - dy).
     *  3. Each iteration: plot the current pixel, then decide to step in x, y,
     *     or both by comparing 2*error against dy and dx.
     *
     * This algorithm uses only integer addition, making it faster than DDA
     * and free from floating-point rounding.
     *
     * @param buffer  Pixel buffer to draw into.
     * @param x1, y1  Start endpoint (integer screen coordinates).
     * @param x2, y2  End endpoint (integer screen coordinates).
     * @param color   Pixel colour to write.
     */
    static void drawLineBresenham(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );
};