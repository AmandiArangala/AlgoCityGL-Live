#pragma once

#include "PixelBuffer.h"

class CircleAlgorithms {
public:
    static void drawCircleMidpoint(
        PixelBuffer& buffer,
        int centerX,
        int centerY,
        int radius,
        Color color
    );

private:
    static void plotCircleSymmetryPoints(
        PixelBuffer& buffer,
        int centerX,
        int centerY,
        int x,
        int y,
        Color color
    );
};