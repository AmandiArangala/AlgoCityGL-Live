/**
 * @file FillAlgorithms.h
 * @brief Declares the Scan-Line Polygon Fill algorithm.
 *
 * Polygon filling converts a closed polygon boundary (defined as a list of
 * vertices) into a solid region of coloured pixels.
 *
 * Scan-Line Fill Algorithm
 * ─────────────────────────
 * The algorithm processes the polygon one horizontal scan line at a time:
 *  1. For each scan-line y, find all edges of the polygon that cross that y.
 *  2. Compute the x-intercept of each crossing edge.
 *  3. Sort intercepts left-to-right.
 *  4. Fill pixels between pairs of intercepts (odd-even / parity rule).
 *
 * The odd-even rule states: a pixel is inside the polygon if a ray cast from
 * that pixel crosses an odd number of polygon edges.  By sorting intercepts
 * and filling between pairs (1st–2nd, 3rd–4th, …) we naturally implement this.
 *
 * Used in AlgoCityGL to fill building footprints and ground surfaces.
 */

#pragma once

#include <vector>
#include "CityArea.h"   // For Vec2.
#include "PixelBuffer.h"

class FillAlgorithms {
public:
    /**
     * @brief Fill a convex or concave polygon using the Scan-Line Fill algorithm.
     *
     * @param buffer   Pixel buffer to write filled pixels into.
     * @param polygon  Ordered list of polygon vertices (any winding order).
     * @param color    Fill colour.
     *
     * @note The polygon must have at least 3 vertices; fewer are silently ignored.
     */
    static void scanLineFillPolygon(
        PixelBuffer& buffer,
        const std::vector<Vec2>& polygon,
        Color color
    );

private:
    /**
     * @brief Draw a horizontal row of pixels from xStart to xEnd at height y.
     *
     * Handles the case where xStart > xEnd by swapping them before filling.
     *
     * @param buffer  Pixel buffer to write into.
     * @param xStart  Left x coordinate (inclusive).
     * @param xEnd    Right x coordinate (inclusive).
     * @param y       Vertical scan-line coordinate.
     * @param color   Pixel colour.
     */
    static void drawHorizontalSpan(
        PixelBuffer& buffer,
        int xStart,
        int xEnd,
        int y,
        Color color
    );
};