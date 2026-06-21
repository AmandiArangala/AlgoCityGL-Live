/**
 * @file FillAlgorithms.cpp
 * @brief Implements the Scan-Line Polygon Fill algorithm.
 *
 * Scan-Line Fill — Step-by-step
 * ──────────────────────────────
 * Given a polygon with vertices V0, V1, …, Vn-1:
 *
 *  1. Find the bounding box (minY … maxY) of all vertices.
 *  2. For each integer scan-line y from minY to maxY:
 *       a. For each edge Vi → V(i+1):
 *            - Skip horizontal edges (they contribute no crossing).
 *            - Use the "lower-inclusive, upper-exclusive" vertex rule to avoid
 *              double-counting at polygon corners.
 *            - Compute the x-intercept using linear interpolation:
 *                  x = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
 *       b. Sort the collected x-intercepts.
 *       c. Fill pixels between pairs: [x[0]…x[1]], [x[2]…x[3]], etc.
 *          (Odd-even fill rule.)
 *
 * Vertex rule (step 2a)
 * ─────────────────────
 * When a scan line passes exactly through a vertex, we include the lower
 * endpoint and exclude the upper endpoint ( y >= yMin && y < yMax ).
 * This ensures each vertex is counted exactly once, preventing double fills
 * at corners where two edges meet.
 */

#include "FillAlgorithms.h"
#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// drawHorizontalSpan — Fill one row of pixels
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fill a horizontal span of pixels at scan-line y from xStart to xEnd.
 *
 * Swaps xStart and xEnd if they arrive in the wrong order so the fill
 * always goes left-to-right regardless of polygon winding.
 */
void FillAlgorithms::drawHorizontalSpan(
    PixelBuffer& buffer,
    int xStart,
    int xEnd,
    int y,
    Color color
) {
    if (xStart > xEnd) {
        std::swap(xStart, xEnd); // Ensure left-to-right ordering.
    }

    // Write one pixel per column in the span.
    for (int x = xStart; x <= xEnd; x++) {
        buffer.putPixel(x, y, color);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// scanLineFillPolygon — Main scan-line fill entry point
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fill a polygon using the Scan-Line Fill algorithm.
 *
 * Works for both convex and concave (non-self-intersecting) polygons.
 */
void FillAlgorithms::scanLineFillPolygon(
    PixelBuffer& buffer,
    const std::vector<Vec2>& polygon,
    Color color
) {
    // A polygon needs at least 3 vertices to enclose an area.
    if (polygon.size() < 3) {
        return;
    }

    // ── Step 1: Compute the vertical bounding box ─────────────────────────────
    int minY = static_cast<int>(polygon[0].y);
    int maxY = static_cast<int>(polygon[0].y);

    for (const Vec2& p : polygon) {
        minY = std::min(minY, static_cast<int>(p.y));
        maxY = std::max(maxY, static_cast<int>(p.y));
    }

    // ── Step 2: Process each scan line ───────────────────────────────────────
    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections; // x-intercepts for this scan line.

        // ── Step 2a: Find all edge intersections ──────────────────────────────
        for (size_t i = 0; i < polygon.size(); i++) {
            Vec2 p1 = polygon[i];
            Vec2 p2 = polygon[(i + 1) % polygon.size()]; // Wrap around to close the polygon.

            // Skip horizontal edges — they lie along the scan line, not crossing it.
            if (static_cast<int>(p1.y) == static_cast<int>(p2.y)) {
                continue;
            }

            float yMin = std::min(p1.y, p2.y);
            float yMax = std::max(p1.y, p2.y);

            // Vertex handling rule:
            // Include the lower endpoint, exclude the upper endpoint.
            // This ensures each vertex is counted exactly once when it falls
            // on a scan line, preventing artefacts at polygon corners.
            if (y >= yMin && y < yMax) {
                // Linear interpolation to find x at this y:
                //   x = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
                float x = p1.x + (static_cast<float>(y) - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                intersections.push_back(static_cast<int>(std::round(x)));
            }
        }

        // ── Step 2b: Sort intercepts left-to-right ─────────────────────────────
        std::sort(intersections.begin(), intersections.end());

        // ── Step 2c: Fill between pairs (odd-even rule) ───────────────────────
        // Pair 0: fill from intersections[0] to intersections[1]
        // Pair 1: fill from intersections[2] to intersections[3] … etc.
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            drawHorizontalSpan(
                buffer,
                intersections[i],     // Left edge of fill span.
                intersections[i + 1], // Right edge of fill span.
                y,
                color
            );
        }
    }
}