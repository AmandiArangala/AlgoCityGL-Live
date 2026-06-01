#include "ClippingAlgorithms.h"
#include <algorithm>

int ClippingAlgorithms::computeOutcode(int x, int y, int xMin, int yMin, int xMax, int yMax) {
    int code = INSIDE;

    if (x < xMin)       code |= LEFT;
    else if (x > xMax)  code |= RIGHT;

    if (y < yMin)       code |= TOP;
    else if (y > yMax)  code |= BOTTOM;

    return code;
}

bool ClippingAlgorithms::cohenSutherland(
    int x1, int y1, int x2, int y2,
    int xMin, int yMin, int xMax, int yMax,
    int& outX1, int& outY1, int& outX2, int& outY2
) {
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
    float dx = x2 - x1;
    float dy = y2 - y1;

    // p and q arrays: 4 boundary tests
    float p[4] = { -dx,  dx, -dy,  dy };
    float q[4] = { x1 - xMin, xMax - x1, y1 - yMin, yMax - y1 };

    float t0 = 0.0f;
    float t1 = 1.0f;

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            // Line is parallel to this boundary
            if (q[i] < 0.0f) {
                // Line is outside this boundary — reject
                return false;
            }
            continue;
        }

        float t = q[i] / p[i];

        if (p[i] < 0.0f) {
            // Line goes from outside to inside
            if (t > t0) t0 = t;
        } else {
            // Line goes from inside to outside
            if (t < t1) t1 = t;
        }

        if (t0 > t1) {
            return false;
        }
    }

    outX1 = x1 + t0 * dx;
    outY1 = y1 + t0 * dy;
    outX2 = x1 + t1 * dx;
    outY2 = y1 + t1 * dy;

    return true;
}
