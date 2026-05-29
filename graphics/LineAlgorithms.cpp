#include "LineAlgorithms.h"
#include <cmath>
#include <algorithm>

void LineAlgorithms::drawLineDDA(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = std::max(std::abs(dx), std::abs(dy));

    if (steps == 0) {
        buffer.putPixel(x1, y1, color);
        return;
    }

    float xIncrement = dx / static_cast<float>(steps);
    float yIncrement = dy / static_cast<float>(steps);

    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    for (int i = 0; i <= steps; i++) {
        buffer.putPixel(
            static_cast<int>(std::round(x)),
            static_cast<int>(std::round(y)),
            color
        );

        x += xIncrement;
        y += yIncrement;
    }
}

void LineAlgorithms::drawLineBresenham(
    PixelBuffer& buffer,
    int x1, int y1,
    int x2, int y2,
    Color color
) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int error = dx - dy;

    while (true) {
        buffer.putPixel(x1, y1, color);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int error2 = 2 * error;

        if (error2 > -dy) {
            error -= dy;
            x1 += sx;
        }

        if (error2 < dx) {
            error += dx;
            y1 += sy;
        }
    }
}