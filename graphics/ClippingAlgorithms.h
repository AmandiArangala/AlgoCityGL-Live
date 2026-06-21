/**
 * @file ClippingAlgorithms.h
 * @brief Declares Cohen-Sutherland and Liang-Barsky line-clipping algorithms.
 *
 * Line clipping trims a line segment so that only the portion inside a
 * rectangular clip window is drawn.  This is essential for:
 *  - Avoiding drawing outside the visible viewport.
 *  - Preventing integer overflow in pixel-buffer coordinates.
 *
 * Two classic algorithms are provided:
 *
 * Cohen-Sutherland (integer version)
 * ───────────────────────────────────
 * Assigns a 4-bit "outcode" to each endpoint indicating which side(s) of
 * the clip window it lies outside.  Uses bitwise AND / OR logic to rapidly
 * accept or reject whole segments before computing intersection points.
 *
 * Liang-Barsky (floating-point version)
 * ──────────────────────────────────────
 * Parameterises the line as P(t) = P1 + t*(P2-P1), 0 ≤ t ≤ 1.
 * Solves for the range of t values that lie inside each of the four clip
 * boundaries (left, right, bottom, top) using p/q inequalities.
 * More efficient than Cohen-Sutherland for lines that partially cross the window.
 */

#pragma once

class ClippingAlgorithms {
public:
    /**
     * @brief Clip a line segment to a rectangle using Cohen-Sutherland.
     *
     * Repeatedly clips the segment against each boundary of the window
     * until the line is either fully inside (trivially accept) or
     * fully outside (trivially reject).
     *
     * @param x1, y1        Start endpoint of the input line.
     * @param x2, y2        End endpoint of the input line.
     * @param xMin, yMin    Top-left corner of the clip window.
     * @param xMax, yMax    Bottom-right corner of the clip window.
     * @param outX1, outY1  Start endpoint of the clipped line (output).
     * @param outX2, outY2  End endpoint of the clipped line (output).
     * @return true if any part of the line is inside the clip window.
     */
    static bool cohenSutherland(
        int x1, int y1, int x2, int y2,
        int xMin, int yMin, int xMax, int yMax,
        int& outX1, int& outY1, int& outX2, int& outY2
    );

    /**
     * @brief Clip a line segment to a rectangle using Liang-Barsky (float).
     *
     * Computes the parameter range [t0, t1] of the line P(t) = P1 + t*(P2-P1)
     * that lies inside the clip window, then reconstructs the clipped endpoints.
     *
     * @param x1, y1        Start endpoint of the input line.
     * @param x2, y2        End endpoint of the input line.
     * @param xMin, yMin    Top-left corner of the clip window.
     * @param xMax, yMax    Bottom-right corner of the clip window.
     * @param outX1, outY1  Start endpoint of the clipped line (output).
     * @param outX2, outY2  End endpoint of the clipped line (output).
     * @return true if any part of the line is inside the clip window.
     */
    static bool liangBarsky(
        float x1, float y1, float x2, float y2,
        float xMin, float yMin, float xMax, float yMax,
        float& outX1, float& outY1, float& outX2, float& outY2
    );

private:
    // ── Outcode bit-flags for Cohen-Sutherland ────────────────────────────────
    // Each bit indicates which side of the clip window an endpoint is outside.
    static const int INSIDE = 0; ///< Point is inside the clip window (all bits clear).
    static const int LEFT   = 1; ///< Point is to the left   of xMin.
    static const int RIGHT  = 2; ///< Point is to the right  of xMax.
    static const int BOTTOM = 4; ///< Point is below         yMax (screen y increases downward).
    static const int TOP    = 8; ///< Point is above         yMin.

    /**
     * @brief Compute the 4-bit outcode for a point relative to the clip window.
     *
     * Sets bits LEFT, RIGHT, TOP, BOTTOM as appropriate.
     * Returns INSIDE (0) if the point is within the window.
     */
    static int computeOutcode(int x, int y, int xMin, int yMin, int xMax, int yMax);
};
