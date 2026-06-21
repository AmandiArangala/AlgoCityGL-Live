/**
 * @file CircleAlgorithms.cpp
 * @brief Implements the Midpoint Circle Algorithm.
 *
 * The Midpoint Circle Algorithm
 * ──────────────────────────────
 * Goal: find all integer pixels that lie on (or nearest to) a circle of
 * radius r centred at (cx, cy).
 *
 * Key insight — 8-fold symmetry
 * ──────────────────────────────
 * A circle is symmetric across the x-axis, y-axis, and both diagonals.
 * If we know one point (x, y) on the circle in the first octant
 * (0 ≤ x ≤ y, i.e., the 45°–90° arc), we can derive 7 more points for free:
 *
 *   (±x, ±y)  and  (±y, ±x)   relative to the centre.
 *
 * So we only need to iterate one octant, then call plotCircleSymmetryPoints()
 * to stamp all 8 reflected pixels at once.
 *
 * Decision variable
 * ─────────────────
 * We start at (x=0, y=r) and walk x rightward (+1 each step).
 * At each step, y either stays the same or decreases by 1 — decided by
 * whether the "midpoint" between the two candidate pixels lies inside or
 * outside the true circle.
 *
 * The decision variable `d` starts at (1 - r):
 *   if d < 0  → midpoint is inside the circle → stay at same y
 *               d_new = d + 2x + 3
 *   if d ≥ 0  → midpoint is outside → decrement y
 *               d_new = d + 2(x - y) + 5
 *
 * No multiplication or division is needed inside the loop — only additions.
 */

#include "CircleAlgorithms.h"

// ─────────────────────────────────────────────────────────────────────────────
// plotCircleSymmetryPoints — Mirror one octant point to all 8 octants
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Plot 8 reflected pixels for a circle point at offset (x, y).
 *
 * Uses the circle's 8-fold symmetry to stamp all reflected pixels at once,
 * so the main loop only has to compute one octant (≈ r/√2 iterations).
 *
 * The 8 points plotted are:
 *   Top/bottom halves via ±y:    (cx+x, cy+y), (cx-x, cy+y),
 *                                 (cx+x, cy-y), (cx-x, cy-y)
 *   Left/right halves via ±x↔y: (cx+y, cy+x), (cx-y, cy+x),
 *                                 (cx+y, cy-x), (cx-y, cy-x)
 */
void CircleAlgorithms::plotCircleSymmetryPoints(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int x,
    int y,
    Color color
) {
    // First four: reflect across both axes using ±x and ±y.
    buffer.putPixel(centerX + x, centerY + y, color);
    buffer.putPixel(centerX - x, centerY + y, color);
    buffer.putPixel(centerX + x, centerY - y, color);
    buffer.putPixel(centerX - x, centerY - y, color);

    // Last four: swap x and y roles (reflect across the two diagonals).
    buffer.putPixel(centerX + y, centerY + x, color);
    buffer.putPixel(centerX - y, centerY + x, color);
    buffer.putPixel(centerX + y, centerY - x, color);
    buffer.putPixel(centerX - y, centerY - x, color);
}

// ─────────────────────────────────────────────────────────────────────────────
// drawCircleMidpoint — Walk one octant using the decision variable
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Rasterize a circle using the Midpoint Circle Algorithm.
 *
 * Iteration:
 *  - Start at (x=0, y=radius).
 *  - Advance x by 1 each step.
 *  - Use `decision` to decide whether y should decrease.
 *  - Stop when x > y (we've covered the full first octant).
 */
void CircleAlgorithms::drawCircleMidpoint(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int radius,
    Color color
) {
    int x = 0;       // Start at the top of the first octant.
    int y = radius;  // y begins at the full radius.

    // Initial decision variable: d = 1 - r
    // Derived from evaluating the midpoint f(x, y) = x² + y² - r² at the
    // very first candidate midpoint (0.5, r - 0.5) rounded to integer form.
    int decision = 1 - radius;

    // Walk from (0, r) to (x == y), covering the first 45° octant.
    while (x <= y) {
        // Stamp all 8 symmetric pixels for the current (x, y).
        plotCircleSymmetryPoints(buffer, centerX, centerY, x, y, color);

        if (decision < 0) {
            // Midpoint is INSIDE the circle → y stays the same, only x advances.
            // Update decision: d_new = d + 2x + 3
            decision += 2 * x + 3;
        } else {
            // Midpoint is ON or OUTSIDE the circle → move y inward (decrement).
            // Update decision: d_new = d + 2(x - y) + 5
            decision += 2 * (x - y) + 5;
            y--;
        }

        x++; // Always advance x by 1 each step.
    }
}