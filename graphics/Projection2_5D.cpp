/**
 * @file Projection2_5D.cpp
 * @brief Implements the isometric (2.5D) projection for the city renderer.
 *
 * Isometric projection math:
 *   Given a flat world point (worldX, worldY), the screen position is:
 *
 *   isoX = (worldX - worldY) * 0.75 + 640
 *   isoY = (worldX + worldY) * 0.375 + 40
 *
 * The factors 0.75 and 0.375 produce the classic 2:1 isometric ratio,
 * and the offsets (640, 40) roughly centre the scene in a 1280x720 window.
 *
 * shiftUp() computes the projected top of a building by subtracting a
 * height-scaled value from the screen Y coordinate:
 *   topY = isoY - height * 0.6
 */
#include "Projection2_5D.h"

Vec2 Projection2_5D::projectPoint(const Vec2& point) {
    // Simple isometric-style projection.
    // These constants are chosen to keep the map visible in the current window.
    float isoX = (point.x - point.y) * 0.75f + 640.0f; // Horizontal isometric offset.
    float isoY = (point.x + point.y) * 0.375f + 40.0f; // Vertical isometric offset.

    return Vec2(isoX, isoY);
}

Vec2 Projection2_5D::shiftUp(const Vec2& point, float height) {
    // Building height goes upward on screen (subtract from y).
    // The factor 0.6 was tuned to match the isometric vertical scale.
    return Vec2(point.x, point.y - height * 0.6f);
}