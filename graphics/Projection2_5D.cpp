#include "Projection2_5D.h"

Vec2 Projection2_5D::projectPoint(const Vec2& point) {
    // Simple isometric-style projection.
    // These constants are chosen to keep the map visible in the current window.
    float isoX = (point.x - point.y) * 0.75f + 640.0f;
    float isoY = (point.x + point.y) * 0.375f + 40.0f;

    return Vec2(isoX, isoY);
}

Vec2 Projection2_5D::shiftUp(const Vec2& point, float height) {
    // Building height goes upward on screen.
    return Vec2(point.x, point.y - height * 0.6f);
}