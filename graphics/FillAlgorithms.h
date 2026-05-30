#pragma once

#include <vector>
#include "CityArea.h"
#include "PixelBuffer.h"

class FillAlgorithms {
public:
    static void scanLineFillPolygon(
        PixelBuffer& buffer,
        const std::vector<Vec2>& polygon,
        Color color
    );

private:
    static void drawHorizontalSpan(
        PixelBuffer& buffer,
        int xStart,
        int xEnd,
        int y,
        Color color
    );
};