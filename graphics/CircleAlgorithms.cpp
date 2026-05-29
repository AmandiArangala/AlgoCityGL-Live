#include "CircleAlgorithms.h"

void CircleAlgorithms::plotCircleSymmetryPoints(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int x,
    int y,
    Color color
) {
    buffer.putPixel(centerX + x, centerY + y, color);
    buffer.putPixel(centerX - x, centerY + y, color);
    buffer.putPixel(centerX + x, centerY - y, color);
    buffer.putPixel(centerX - x, centerY - y, color);

    buffer.putPixel(centerX + y, centerY + x, color);
    buffer.putPixel(centerX - y, centerY + x, color);
    buffer.putPixel(centerX + y, centerY - x, color);
    buffer.putPixel(centerX - y, centerY - x, color);
}

void CircleAlgorithms::drawCircleMidpoint(
    PixelBuffer& buffer,
    int centerX,
    int centerY,
    int radius,
    Color color
) {
    int x = 0;
    int y = radius;

    int decision = 1 - radius;

    while (x <= y) {
        plotCircleSymmetryPoints(buffer, centerX, centerY, x, y, color);

        if (decision < 0) {
            decision += 2 * x + 3;
        } else {
            decision += 2 * (x - y) + 5;
            y--;
        }

        x++;
    }
}