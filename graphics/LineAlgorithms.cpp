/**
 * @file LineAlgorithms.cpp
 * @brief Implements DDA and Bresenham line-drawing rasterisation algorithms.
 *
 * DDA (Digital Differential Analyzer)
 * ─────────────────────────────────────
 * Steps along the major axis one pixel at a time using floating-point increments.
 * - Advantages  : Simple code, works for all slopes.
 * - Disadvantages: Floating-point rounding can introduce sub-pixel error.
 * - Time complexity: O(max(|dx|, |dy|)) per line.
 *
 * Bresenham Line Algorithm
 * ─────────────────────────
 * Uses only integer arithmetic and an error accumulator.
 * - Advantages  : Faster than DDA (no floats), pixel-perfect on integer grids.
 * - Disadvantages: Slightly more complex logic with sx/sy step directions.
 * - Time complexity: O(max(|dx|, |dy|)) per line.
 */
#include "LineAlgorithms.h"
#include <cmath>
#include <algorithm>

void LineAlgorithms::drawLineDDA(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    // Compute the change in x and y across the whole line.
    int dx = x2 - x1;
    int dy = y2 - y1;

    // Choose the larger absolute delta as the number of steps so we
    // advance at most 1 pixel along each axis per iteration.
    int steps = std::max(std::abs(dx), std::abs(dy));

    // Degenerate case: both endpoints are the same pixel.
    if (steps == 0) {
        buffer.putPixel(x1, y1, color);
        return;
    }

    // Compute the floating-point increment per step along each axis.
    float xIncrement = dx / static_cast<float>(steps);
    float yIncrement = dy / static_cast<float>(steps);

    // Start at the first endpoint (as float for sub-pixel accuracy).
    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    // Walk along the line, rounding to the nearest integer pixel each step.
    for (int i = 0; i <= steps; i++) {
        buffer.putPixel(
            static_cast<int>(std::round(x)),
            static_cast<int>(std::round(y)),
            color
        );

        x += xIncrement;
        y += yIncrement;
    }
}

void LineAlgorithms::drawLineBresenham(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    // |dx| and |dy| (always positive) control the step rate in each axis.
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    // Step direction: +1 or -1 per axis depending on which way the line goes.
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    // error accumulates the deviation between the ideal line and the raster grid.
    // Initialised to dx - dy so the first step is correct for any slope.
    int error = dx - dy;

    while (true) {
        buffer.putPixel(x1, y1, color); // Plot the current pixel.

        // Stop once we reach the destination endpoint.
        if (x1 == x2 && y1 == y2) {
            break;
        }

        // Double the error to avoid fractional comparisons.
        int error2 = 2 * error;

        // If error2 > -dy we have overshot in x — step in the x direction.
        if (error2 > -dy) {
            error -= dy;
            x1 += sx;
        }

        // If error2 < dx we have not yet reached the next y — step in y.
        if (error2 < dx) {
            error += dx;
            y1 += sy;
        }
    }
}