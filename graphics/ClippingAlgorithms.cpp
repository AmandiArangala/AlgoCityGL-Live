/**
 * @file ClippingAlgorithms.cpp
 * @brief Implements Cohen-Sutherland and Liang-Barsky line clipping.
 *
 * ── Cohen-Sutherland ──────────────────────────────────────────────────────────
 *
 * Each endpoint is assigned a 4-bit outcode:
 *   Bit 0 (LEFT)   = 1 if x < xMin
 *   Bit 1 (RIGHT)  = 1 if x > xMax
 *   Bit 2 (BOTTOM) = 1 if y > yMax   (screen y grows downward)
 *   Bit 3 (TOP)    = 1 if y < yMin
 *
 * The loop applies two fast tests each iteration:
 *   • code1 | code2 == 0  → both inside → trivially accept.
 *   • code1 & code2 != 0  → share an outside region → trivially reject.
 * Otherwise, clip the outside endpoint against one boundary and repeat.
 *
 * ── Liang-Barsky ─────────────────────────────────────────────────────────────
 *
 * Parametrize the line: P(t) = P1 + t*(P2 - P1),  t ∈ [0, 1].
 *
 * For each of the four clip boundaries, we compute p and q:
 *   Left:   p = -dx,  q = x1 - xMin
 *   Right:  p =  dx,  q = xMax - x1
 *   Bottom: p = -dy,  q = y1 - yMin
 *   Top:    p =  dy,  q = yMax - y1
 *
 * Then t = q/p gives the parameter of intersection with that boundary.
 *   p < 0 → line enters the boundary → update t0 = max(t0, t)
 *   p > 0 → line exits the boundary  → update t1 = min(t1, t)
 *   p = 0, q < 0 → line is parallel AND outside → reject immediately
 *
 * After processing all four boundaries:
 *   if t0 > t1 → the line is outside → reject.
 *   else → the clipped endpoints are P(t0) and P(t1).
 */

#include "ClippingAlgorithms.h"
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// computeOutcode — Compute the 4-bit region code for a point
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Return the Cohen-Sutherland outcode for point (x, y) against the window.
 *
 * Builds the code by OR-ing in each boundary bit that the point violates.
 * A code of INSIDE (0) means the point is completely within the window.
 */
int ClippingAlgorithms::computeOutcode(int x, int y, int xMin, int yMin, int xMax, int yMax) {
    int code = INSIDE; // Start with "inside" and set bits for each violated boundary.

    if (x < xMin)       code |= LEFT;   // Point is left of the window.
    else if (x > xMax)  code |= RIGHT;  // Point is right of the window.

    if (y < yMin)       code |= TOP;    // Point is above the window (y=0 at top).
    else if (y > yMax)  code |= BOTTOM; // Point is below the window.

    return code;
}

// ─────────────────────────────────────────────────────────────────────────────
// cohenSutherland — Clip a line segment using outcode logic
// ─────────────────────────────────────────────────────────────────────────────

bool ClippingAlgorithms::cohenSutherland(
    int x1, int y1, int x2, int y2,
    int xMin, int yMin, int xMax, int yMax,
    int& outX1, int& outY1, int& outX2, int& outY2
) {
    // Compute initial outcodes for both endpoints.
    int code1 = computeOutcode(x1, y1, xMin, yMin, xMax, yMax);
    int code2 = computeOutcode(x2, y2, xMin, yMin, xMax, yMax);

    bool accepted = false;

    while (true) {
        if (!(code1 | code2)) {
            // Both endpoints inside — trivially accept the whole segment.
            accepted = true;
            break;
        }

        if (code1 & code2) {
            // Both endpoints share at least one outside region — trivially reject.
            // The segment cannot possibly intersect the clip window.
            break;
        }

        // At least one endpoint is outside and they do NOT share an outside region.
        // Pick one outside endpoint (prefer code1 if it's outside, else use code2).
        int codeOut = code1 ? code1 : code2;

        int x = 0, y = 0; // New clipped position for the outside endpoint.

        // Clip against the boundary corresponding to the highest-set bit.
        if (codeOut & BOTTOM) {
            // Clip to bottom edge (y = yMax).
            // Solve for x where the line crosses y = yMax:
            //   x = x1 + (x2-x1) * (yMax - y1) / (y2 - y1)
            x = x1 + (x2 - x1) * (yMax - y1) / (y2 - y1);
            y = yMax;
        } else if (codeOut & TOP) {
            // Clip to top edge (y = yMin).
            x = x1 + (x2 - x1) * (yMin - y1) / (y2 - y1);
            y = yMin;
        } else if (codeOut & RIGHT) {
            // Clip to right edge (x = xMax).
            y = y1 + (y2 - y1) * (xMax - x1) / (x2 - x1);
            x = xMax;
        } else if (codeOut & LEFT) {
            // Clip to left edge (x = xMin).
            y = y1 + (y2 - y1) * (xMin - x1) / (x2 - x1);
            x = xMin;
        }

        // Replace the outside endpoint with the newly clipped point,
        // then recompute its outcode and loop again.
        if (codeOut == code1) {
            x1    = x; y1    = y;
            code1 = computeOutcode(x1, y1, xMin, yMin, xMax, yMax);
        } else {
            x2    = x; y2    = y;
            code2 = computeOutcode(x2, y2, xMin, yMin, xMax, yMax);
        }
    }

    // Copy results only if the segment was accepted.
    if (accepted) {
        outX1 = x1; outY1 = y1;
        outX2 = x2; outY2 = y2;
    }

    return accepted;
}

// ─────────────────────────────────────────────────────────────────────────────
// liangBarsky — Clip a floating-point line using parameter intervals
// ─────────────────────────────────────────────────────────────────────────────

bool ClippingAlgorithms::liangBarsky(
    float x1, float y1, float x2, float y2,
    float xMin, float yMin, float xMax, float yMax,
    float& outX1, float& outY1, float& outX2, float& outY2
) {
    float dx = x2 - x1; // Horizontal component of the line direction vector.
    float dy = y2 - y1; // Vertical component of the line direction vector.

    // p[i] and q[i] encode each boundary's clipping condition:
    //   The line is inside boundary i when:  p[i]*t >= -q[i]   (for p[i] != 0)
    //
    //   i=0: Left boundary   p=-dx, q=x1-xMin
    //   i=1: Right boundary  p=+dx, q=xMax-x1
    //   i=2: Bottom boundary p=-dy, q=y1-yMin
    //   i=3: Top boundary    p=+dy, q=yMax-y1
    float p[4] = { -dx,  dx, -dy,  dy };
    float q[4] = { x1 - xMin, xMax - x1, y1 - yMin, yMax - y1 };

    // t0 and t1 track the entering and exiting parameter values.
    // Initially the whole line [0, 1] is a candidate.
    float t0 = 0.0f; // Parameter at the entering intersection.
    float t1 = 1.0f; // Parameter at the exiting intersection.

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            // Line is parallel to this boundary.
            if (q[i] < 0.0f) {
                // Line is parallel AND outside this boundary — reject entirely.
                return false;
            }
            // Parallel and inside → this boundary doesn't constrain t; skip.
            continue;
        }

        float t = q[i] / p[i]; // Parameter value where line crosses this boundary.

        if (p[i] < 0.0f) {
            // Negative p → line goes from outside to inside (entering).
            // Update the entering parameter to be the latest (largest) entry.
            if (t > t0) t0 = t;
        } else {
            // Positive p → line goes from inside to outside (exiting).
            // Update the exiting parameter to be the earliest (smallest) exit.
            if (t < t1) t1 = t;
        }

        if (t0 > t1) {
            // The entering parameter has passed the exiting parameter →
            // no valid interval remains → the line is entirely outside.
            return false;
        }
    }

    // Reconstruct the clipped endpoints from the surviving [t0, t1] interval.
    outX1 = x1 + t0 * dx;
    outY1 = y1 + t0 * dy;
    outX2 = x1 + t1 * dx;
    outY2 = y1 + t1 * dy;

    return true;
}
