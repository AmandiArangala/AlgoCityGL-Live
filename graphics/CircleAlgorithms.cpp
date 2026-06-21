/**
 * @file CircleAlgorithms.cpp
 * @brief Implements the Midpoint Circle Algorithm.
 *
 * Midpoint Circle Algorithm — how it works:
 * ─────────────────────────────────────────
 * Goal: find all integer pixels that lie on the border of a circle of
 * given radius centered at (centerX, centerY).
 *
 * 1. Start in the first octant at (x=0, y=radius).
 * 2. Maintain a decision variable:  decision = 1 - radius.
 *    - If decision < 0  → the midpoint is inside the circle → step only in x.
 *      Update: decision += 2*x + 3
 *    - If decision >= 0 → the midpoint is outside → step in x and decrement y.
 *      Update: decision += 2*(x-y) + 5
 * 3. For each (x, y) computed, plot 8 symmetric points simultaneously.
 * 4. Continue until x > y (one full octant processed).
 *
 * Only integer arithmetic is used throughout — no floating-point operations.
 */
#include "CircleAlgorithms.h"

void CircleAlgorithms::plotCircleSymmetryPoints(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int x,
    int y,
    Color color
) {
    // A circle has 8-fold symmetry: each computed (x,y) in one octant
    // corresponds to 7 other points in the remaining octants.
    // The 8 pixels are at (\u00b1x, \u00b1y) and (\u00b1y, \u00b1x) relative to the centre.
    buffer.putPixel(centerX + x, centerY + y, color); // Octant 1
    buffer.putPixel(centerX - x, centerY + y, color); // Octant 4
    buffer.putPixel(centerX + x, centerY - y, color); // Octant 8
    buffer.putPixel(centerX - x, centerY - y, color); // Octant 5

    buffer.putPixel(centerX + y, centerY + x, color); // Octant 2
    buffer.putPixel(centerX - y, centerY + x, color); // Octant 3
    buffer.putPixel(centerX + y, centerY - x, color); // Octant 7
    buffer.putPixel(centerX - y, centerY - x, color); // Octant 6
}

void CircleAlgorithms::drawCircleMidpoint(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int radius,
    Color color
) {
    int x = 0;       // Start at the top of the first octant.
    int y = radius;  // y begins at the full radius.

    // Initial decision variable: 1 - r  (derived from the circle equation).
    int decision = 1 - radius;

    // Loop until we have processed the first octant (x crosses y).
    while (x <= y) {
        // Plot 8 symmetric points for this (x, y) offset.
        plotCircleSymmetryPoints(buffer, centerX, centerY, x, y, color);

        if (decision < 0) {
            // Midpoint is still inside the circle: step only east (+x).
            decision += 2 * x + 3;
        } else {
            // Midpoint is outside or on the circle: step east and south (+x, -y).
            decision += 2 * (x - y) + 5;
            y--;
        }

        x++; // Always advance x.
    }
}