/**
 * @file FillAlgorithms.h
 * @brief Declares the Scan-Line Polygon Fill algorithm.
 *
 * Polygon filling converts a closed polygon (list of vertices) into a solid
 * filled region of pixels.
 *
 * Scan-Line Fill algorithm summary:
 *  1. Find the Y range [minY, maxY] of the polygon.
 *  2. For each horizontal scan line y in that range:
 *     a. Find all X intersections of the scan line with polygon edges.
 *     b. Sort intersections from left to right.
 *     c. Fill pixels between each pair of intersections (odd-even rule).
 */
#pragma once

#include <vector>
#include "CityArea.h"
#include "PixelBuffer.h"

class FillAlgorithms {
public:
    /**
     * @brief Fills a convex or concave polygon using the scan-line algorithm.
     * @param buffer   Destination PixelBuffer.
     * @param polygon  List of vertices in world/screen coordinates.
     * @param color    Fill colour.
     * @note Requires at least 3 vertices.  Degenerate polygons are skipped.
     */
    static void scanLineFillPolygon(
        PixelBuffer& buffer,
        const std::vector<Vec2>& polygon,
        Color color
    );

private:
    /**
     * @brief Fills a horizontal run of pixels from xStart to xEnd on scan line y.
     * @note Automatically swaps xStart and xEnd if they are out of order.
     */
    static void drawHorizontalSpan(
        PixelBuffer& buffer,
        int xStart,
        int xEnd,
        int y,
        Color color
    );
};