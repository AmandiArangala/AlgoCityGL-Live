#include "FillAlgorithms.h"
#include <algorithm>
#include <cmath>

void FillAlgorithms::drawHorizontalSpan(
    PixelBuffer& buffer,
    int xStart,
    int xEnd,
    int y,
    Color color
) {
    if (xStart > xEnd) {
        std::swap(xStart, xEnd);
    }

    for (int x = xStart; x <= xEnd; x++) {
        buffer.putPixel(x, y, color);
    }
}

void FillAlgorithms::scanLineFillPolygon(
    PixelBuffer& buffer,
    const std::vector<Vec2>& polygon,
    Color color
) {
    if (polygon.size() < 3) {
        return;
    }

    int minY = static_cast<int>(polygon[0].y);
    int maxY = static_cast<int>(polygon[0].y);

    for (const Vec2& p : polygon) {
        minY = std::min(minY, static_cast<int>(p.y));
        maxY = std::max(maxY, static_cast<int>(p.y));
    }

    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections;

        for (size_t i = 0; i < polygon.size(); i++) {
            Vec2 p1 = polygon[i];
            Vec2 p2 = polygon[(i + 1) % polygon.size()];

            // Ignore horizontal edges
            if (static_cast<int>(p1.y) == static_cast<int>(p2.y)) {
                continue;
            }

            float yMin = std::min(p1.y, p2.y);
            float yMax = std::max(p1.y, p2.y);

            // Vertex handling rule:
            // include lower endpoint, exclude upper endpoint
            if (y >= yMin && y < yMax) {
                float x = p1.x + (static_cast<float>(y) - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                intersections.push_back(static_cast<int>(std::round(x)));
            }
        }

        std::sort(intersections.begin(), intersections.end());

        // Odd-even rule: fill between pairs
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            drawHorizontalSpan(
                buffer,
                intersections[i],
                intersections[i + 1],
                y,
                color
            );
        }
    }
}