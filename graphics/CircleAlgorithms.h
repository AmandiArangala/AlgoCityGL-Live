/**
 * @file CircleAlgorithms.h
 * @brief Declares the Midpoint Circle Algorithm for rasterising circles.
 *
 * The Midpoint Circle Algorithm (also known as Bresenham's Circle Algorithm)
 * exploits the 8-fold symmetry of a circle.  It computes one octant of pixels
 * and mirrors them into all 8 octants simultaneously, reducing the number of
 * pixels computed by a factor of 8.
 *
 * Only integer arithmetic is used — no trigonometry or square roots in the
 * main loop — making it very efficient.
 */
#pragma once

#include "PixelBuffer.h"

class CircleAlgorithms {
public:
    /**
     * @brief Draws the outline of a circle using the Midpoint Circle Algorithm.
     * @param buffer   Destination PixelBuffer.
     * @param centerX  Screen x coordinate of the circle centre.
     * @param centerY  Screen y coordinate of the circle centre.
     * @param radius   Radius in pixels.
     * @param color    Fill colour for each border pixel.
     */
    static void drawCircleMidpoint(
        PixelBuffer& buffer,
        int centerX,
        int centerY,
        int radius,
        Color color
    );

private:
    /**
     * @brief Plots 8 symmetric points given one (x,y) offset from the centre.
     *
     * Because a circle is symmetric in all 8 octants, one (x,y) sample gives
     * 8 pixels at (\u00b1x, \u00b1y) and (\u00b1y, \u00b1x).
     */
    static void plotCircleSymmetryPoints(
        PixelBuffer& buffer,
        int centerX,
        int centerY,
        int x,
        int y,
        Color color
    );
};