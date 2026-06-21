/**
 * @file FillAlgorithms.cpp
 * @brief Implements the Scan-Line Polygon Fill algorithm.
 *
 * Scan-Line Fill — step by step:
 * 1. Find the vertical range [minY, maxY] of the polygon bounding box.
 * 2. For each horizontal scan line y from minY to maxY:
 *    a. Iterate every polygon edge.
 *    b. Skip horizontal edges (they do not create intersections).
 *    c. For edges that span this y, compute the exact x intersection using
 *       linear interpolation: x = x1 + (y - y1) * (x2-x1)/(y2-y1).
 *    d. Use the \"lower endpoint included, upper excluded\" rule to handle
 *       shared vertices correctly (avoids double-counting at corners).
 * 3. Sort all intersection x values left to right.
 * 4. Apply the odd-even (parity) rule: fill between intersection pairs
 *    (pair 0-1, pair 2-3, etc.).
 */
#include "FillAlgorithms.h"
#include <algorithm>
#include <cmath>

void FillAlgorithms::drawHorizontalSpan(
    PixelBuffer& buffer,
    int xStart,
    int xEnd,
    int y,
    Color color
) {
    // Ensure we always iterate left-to-right, regardless of intersection order.
    if (xStart > xEnd) {
        std::swap(xStart, xEnd);
    }

    // Fill every pixel in the horizontal run from xStart to xEnd inclusive.
    for (int x = xStart; x <= xEnd; x++) {
        buffer.putPixel(x, y, color);
    }
}

void FillAlgorithms::scanLineFillPolygon(
    PixelBuffer& buffer,
    const std::vector<Vec2>& polygon,
    Color color
) {
    // A polygon needs at least 3 vertices to enclose area.
    if (polygon.size() < 3) {
        return;
    }

    // Step 1: find the vertical bounding range of the polygon.
    int minY = static_cast<int>(polygon[0].y);
    int maxY = static_cast<int>(polygon[0].y);

    for (const Vec2& p : polygon) {
        minY = std::min(minY, static_cast<int>(p.y));
        maxY = std::max(maxY, static_cast<int>(p.y));
    }

    // Step 2: process each horizontal scan line.
    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections; // X positions where this scan line crosses edges.

        // Step 2a: test every polygon edge.
        for (size_t i = 0; i < polygon.size(); i++) {
            Vec2 p1 = polygon[i];
            Vec2 p2 = polygon[(i + 1) % polygon.size()]; // Wrap to close the polygon.

            // Step 2b: ignore perfectly horizontal edges (they don't create crossings).
            if (static_cast<int>(p1.y) == static_cast<int>(p2.y)) {
                continue;
            }

            float yMin = std::min(p1.y, p2.y);
            float yMax = std::max(p1.y, p2.y);

            // Step 2c & 2d: include the lower endpoint, exclude the upper endpoint.
            // This ensures shared vertices at corners are counted exactly once.
            if (y >= yMin && y < yMax) {
                // Linear interpolation: find x where the edge crosses scan line y.
                float x = p1.x + (static_cast<float>(y) - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                intersections.push_back(static_cast<int>(std::round(x)));
            }
        }

        // Step 3: sort intersections left to right.
        std::sort(intersections.begin(), intersections.end());

        // Step 4 (odd-even rule): fill between each consecutive pair of intersections.
        // Pair [0,1], [2,3], etc. are inside the polygon.
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            drawHorizontalSpan(
                buffer,
                intersections[i],
                intersections[i + 1],
                y,
                color
            );
        }
    }
}