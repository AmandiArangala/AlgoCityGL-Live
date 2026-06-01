#pragma once

class ClippingAlgorithms {
public:
    // Cohen-Sutherland line clipping.
    // Returns true if any part of the line is inside the clip window.
    // outX1/outY1/outX2/outY2 receive the clipped endpoints.
    static bool cohenSutherland(
        int x1, int y1, int x2, int y2,
        int xMin, int yMin, int xMax, int yMax,
        int& outX1, int& outY1, int& outX2, int& outY2
    );

    // Liang-Barsky line clipping (floating-point version).
    static bool liangBarsky(
        float x1, float y1, float x2, float y2,
        float xMin, float yMin, float xMax, float yMax,
        float& outX1, float& outY1, float& outX2, float& outY2
    );

private:
    // Outcode bits for Cohen-Sutherland
    static const int INSIDE = 0;
    static const int LEFT   = 1;
    static const int RIGHT  = 2;
    static const int BOTTOM = 4;
    static const int TOP    = 8;

    static int computeOutcode(int x, int y, int xMin, int yMin, int xMax, int yMax);
};
