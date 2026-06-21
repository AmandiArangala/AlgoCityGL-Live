/**
 * @file LineAlgorithms.h
 * @brief Declares two classic rasterisation algorithms for drawing lines.
 *
 * Both algorithms convert a mathematical line segment defined by two endpoints
 * into a discrete set of pixels stored in a PixelBuffer.
 *
 *  - **DDA (Digital Differential Analyzer)**: Simple float-based incremental method.
 *    Uses floating-point arithmetic to step along the major axis.
 *
 *  - **Bresenham**: Integer-only method using an error accumulator.
 *    Faster and avoids floating-point rounding, making it the preferred
 *    algorithm for real hardware renderers.
 */
#pragma once

#include "PixelBuffer.h"

class LineAlgorithms {
public:
    /**
     * @brief Draws a line from (x1,y1) to (x2,y2) using the DDA algorithm.
     *
     * Steps:
     *  1. Compute dx = x2-x1,  dy = y2-y1.
     *  2. steps = max(|dx|, |dy|)  — ensures we step one pixel at a time.
     *  3. Increment x and y by dx/steps and dy/steps each iteration,
     *     rounding to the nearest integer pixel each step.
     *
     * @param buffer  Destination PixelBuffer.
     * @param x1,y1  Starting endpoint (screen pixels).
     * @param x2,y2  Ending endpoint   (screen pixels).
     * @param color   Fill colour.
     */
    static void drawLineDDA(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );

    /**
     * @brief Draws a line from (x1,y1) to (x2,y2) using Bresenham's algorithm.
     *
     * Uses only integer addition and comparison:
     *  - Maintains an \"error\" term tracking accumulated deviation from the ideal line.
     *  - Decides whether to step diagonally or axially based on the sign of the error.
     *
     * @param buffer  Destination PixelBuffer.
     * @param x1,y1  Starting endpoint (screen pixels).
     * @param x2,y2  Ending endpoint   (screen pixels).
     * @param color   Fill colour.
     */
    static void drawLineBresenham(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );
};