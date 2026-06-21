/**
 * @file LineAlgorithms.cpp
 * @brief Implements DDA and Bresenham line-drawing algorithms.
 *
 * These are two of the most fundamental algorithms in computer graphics.
 * They solve the same problem — rasterizing a line segment into pixels —
 * but use different mathematical approaches.
 *
 * Key concept: pixel grid
 * ───────────────────────
 * Screen pixels live at integer coordinates.  A mathematical line between
 * two integer points passes through many non-integer (x, y) positions; both
 * algorithms decide which integer pixel is closest to the true line at each step.
 */

#include "LineAlgorithms.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// DDA — Digital Differential Analyzer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Rasterize a line from (x1, y1) to (x2, y2) using the DDA algorithm.
 *
 * Theory
 * ──────
 * The DDA divides the line into `steps` equal intervals, where
 *   steps = max(|dx|, |dy|)
 *
 * This ensures we step along the dominant axis one pixel at a time (so no
 * gaps appear), while the minor axis advances fractionally.
 *
 * Each iteration adds a constant floating-point increment to both x and y,
 * then rounds to the nearest integer to identify the pixel to colour.
 *
 * Complexity: O(max(|dx|, |dy|)) — linear in the number of pixels drawn.
 */
void LineAlgorithms::drawLineDDA(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    int dx = x2 - x1; // Total horizontal distance.
    int dy = y2 - y1; // Total vertical distance.

    // The number of steps equals the larger delta so we never skip pixels.
    int steps = std::max(std::abs(dx), std::abs(dy));

    // Degenerate case: both endpoints are the same pixel.
    if (steps == 0) {
        buffer.putPixel(x1, y1, color);
        return;
    }

    // Per-step floating-point increments along each axis.
    float xIncrement = dx / static_cast<float>(steps);
    float yIncrement = dy / static_cast<float>(steps);

    // Start at the first endpoint (as float so we can accumulate fractions).
    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    // Walk along the line, rounding to the nearest integer pixel at each step.
    for (int i = 0; i <= steps; i++) {
        buffer.putPixel(
            static_cast<int>(std::round(x)),
            static_cast<int>(std::round(y)),
            color
        );

        x += xIncrement; // Advance fractionally along x.
        y += yIncrement; // Advance fractionally along y.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Bresenham's Line Algorithm
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Rasterize a line from (x1, y1) to (x2, y2) using Bresenham's algorithm.
 *
 * Theory
 * ──────
 * Bresenham's algorithm avoids floating-point arithmetic entirely.  It maintains
 * an integer `error` term that tracks how far the ideal line has drifted from the
 * current pixel centre.
 *
 * Initially:  error = |dx| - |dy|
 *
 * Each iteration:
 *   error2 = 2 * error    (pre-multiply to avoid division)
 *   if error2 > -|dy|  → step in x, subtract |dy| from error
 *   if error2 < |dx|   → step in y, add    |dx| to error
 *
 * The combined conditions allow diagonal steps (both x and y change) when
 * the slope is exactly ±1.
 *
 * The step directions sx and sy are determined once at the start:
 *   sx = +1 if x1 < x2, else -1
 *   sy = +1 if y1 < y2, else -1
 * This handles all eight octants of slope (positive/negative, steep/shallow).
 *
 * Complexity: O(max(|dx|, |dy|)) — same as DDA, but using only integers.
 */
void LineAlgorithms::drawLineBresenham(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    int dx = std::abs(x2 - x1); // |Δx| — magnitude of horizontal span.
    int dy = std::abs(y2 - y1); // |Δy| — magnitude of vertical span.

    // sx/sy are the step directions (+1 or -1) for each axis.
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    // Initial error term: positive means we're ahead on x, negative on y.
    int error = dx - dy;

    while (true) {
        buffer.putPixel(x1, y1, color); // Plot the current pixel.

        // Stop once we reach the final endpoint.
        if (x1 == x2 && y1 == y2) {
            break;
        }

        int error2 = 2 * error; // Double the error to avoid fractional comparisons.

        // Decide whether to step horizontally.
        // Condition: the line's ideal position is closer to x1+sx than x1.
        if (error2 > -dy) {
            error -= dy; // Reduce error by the y-span.
            x1    += sx; // Step in x.
        }

        // Decide whether to step vertically.
        // Condition: the line's ideal position is closer to y1+sy than y1.
        if (error2 < dx) {
            error += dx; // Increase error by the x-span.
            y1    += sy; // Step in y.
        }
    }
}