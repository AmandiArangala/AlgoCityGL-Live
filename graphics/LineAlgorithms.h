#pragma once

#include "PixelBuffer.h"

class LineAlgorithms {
public:
    static void drawLineDDA(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );

    static void drawLineBresenham(
        PixelBuffer& buffer,
        int x1, int y1,
        int x2, int y2,
        Color color
    );
};