/**
 * @file ClippingAlgorithms.cpp
 * @brief Implements Cohen-Sutherland and Liang-Barsky line clipping algorithms.
 *
 * Cohen-Sutherland (integer version)
 * ────────────────────────────────────
 * Each endpoint is assigned a 4-bit outcode describing its position
 * relative to the clip rectangle: LEFT | RIGHT | BOTTOM | TOP.
 * - If code1 | code2 == 0  → trivially accept (both inside).
 * - If code1 & code2 != 0  → trivially reject (both on same side).
 * - Otherwise, clip one endpoint against the boundary indicated by the outcode.
 * Repeat until trivially accepted or rejected.
 *
 * Liang-Barsky (floating-point version)
 * ──────────────────────────────────────
 * Parameterises the line as P(t) = P1 + t*(P2-P1), t in [0,1].
 * Uses p[] and q[] arrays to test each boundary:
 *   p[i] < 0  → entering boundary   → update t0 = max(t0, q/p).
 *   p[i] > 0  → exiting  boundary   → update t1 = min(t1, q/p).
 *   p[i] = 0  → parallel; reject if q < 0 (outside).
 * If t0 > t1 after all boundaries → reject entirely.
 */
#include "ClippingAlgorithms.h"
#include <algorithm>

int ClippingAlgorithms::computeOutcode(int x, int y, int xMin, int yMin, int xMax, int yMax) {
    int code = INSIDE; // Start assuming the point is inside the clip window.

    // Set the LEFT bit if x is to the left of the left boundary.
    if (x < xMin)       code |= LEFT;
    // Set the RIGHT bit if x is to the right of the right boundary.
    else if (x > xMax)  code |= RIGHT;

    // Set the TOP bit if y is above the top boundary (y increases downward on screen).
    if (y < yMin)       code |= TOP;
    // Set the BOTTOM bit if y is below the bottom boundary.
    else if (y > yMax)  code |= BOTTOM;

    return code;
}

bool ClippingAlgorithms::cohenSutherland(
    int x1, int y1, int x2, int y2,
    int xMin, int yMin, int xMax, int yMax,
    int& outX1, int& outY1, int& outX2, int& outY2
) {
    // Compute the outcode for both endpoints.
    int code1 = computeOutcode(x1, y1, xMin, yMin, xMax, yMax);
    int code2 = computeOutcode(x2, y2, xMin, yMin, xMax, yMax);

    bool accepted = false;

    while (true) {
        if (!(code1 | code2)) {
            // Both endpoints inside — trivially accept
            accepted = true;
            break;
        }

        if (code1 & code2) {
            // Both endpoints share an outside region — trivially reject
            break;
        }

        // Pick an endpoint outside the window
        int codeOut = code1 ? code1 : code2;

        int x = 0, y = 0;

        if (codeOut & BOTTOM) {
            // Clip to bottom edge (y = yMax)
            x = x1 + (x2 - x1) * (yMax - y1) / (y2 - y1);
            y = yMax;
        } else if (codeOut & TOP) {
            // Clip to top edge (y = yMin)
            x = x1 + (x2 - x1) * (yMin - y1) / (y2 - y1);
            y = yMin;
        } else if (codeOut & RIGHT) {
            y = y1 + (y2 - y1) * (xMax - x1) / (x2 - x1);
            x = xMax;
        } else if (codeOut & LEFT) {
            y = y1 + (y2 - y1) * (xMin - x1) / (x2 - x1);
            x = xMin;
        }

        // Replace the outside endpoint with the clipped point.
        if (codeOut == code1) {
            x1 = x;
            y1 = y;
            code1 = computeOutcode(x1, y1, xMin, yMin, xMax, yMax);
        } else {
            x2 = x;
            y2 = y;
            code2 = computeOutcode(x2, y2, xMin, yMin, xMax, yMax);
        }
    }

    if (accepted) {
        outX1 = x1; outY1 = y1;
        outX2 = x2; outY2 = y2;
    }

    return accepted;
}

bool ClippingAlgorithms::liangBarsky(
    float x1, float y1, float x2, float y2,
    float xMin, float yMin, float xMax, float yMax,
    float& outX1, float& outY1, float& outX2, float& outY2
) {
    float dx = x2 - x1; // Horizontal component of the line direction.
    float dy = y2 - y1; // Vertical component of the line direction.

    // p and q arrays encode the four boundary tests.
    // p[i] < 0 means we are entering boundary i; p[i] > 0 means exiting.
    float p[4] = { -dx,  dx, -dy,  dy };
    float q[4] = { x1 - xMin, xMax - x1, y1 - yMin, yMax - y1 };

    // t0 and t1 define the visible portion of the line [0,1].
    float t0 = 0.0f; // Start of visible segment.
    float t1 = 1.0f; // End of visible segment.

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            // Line is parallel to this boundary
            if (q[i] < 0.0f) {
                // Line is outside this boundary — reject
                return false;
            }
            continue;
        }

        float t = q[i] / p[i]; // Parameter value at the boundary intersection.

        if (p[i] < 0.0f) {
            // Line goes from outside to inside — update entering parameter t0.
            if (t > t0) t0 = t;
        } else {
            // Line goes from inside to outside — update exiting parameter t1.
            if (t < t1) t1 = t;
        }

        if (t0 > t1) {
            // No visible segment remains — reject the line.
            return false;
        }
    }

    // Compute the clipped endpoints using the parameterised line equation.
    outX1 = x1 + t0 * dx;
    outY1 = y1 + t0 * dy;
    outX2 = x1 + t1 * dx;
    outY2 = y1 + t1 * dy;

    return true;
}
