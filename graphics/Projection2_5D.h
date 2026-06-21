/**
 * @file Projection2_5D.h
 * @brief Declares the isometric projection used for 2.5D city rendering.
 *
 * 2.5D (also called "isometric" or "dimetric") projection gives a three-
 * dimensional appearance to a 2D scene by projecting the flat (x, y) city map
 * onto a tilted plane.  It is not true 3D — there is no perspective correction —
 * but it creates the illusion of depth at low computational cost.
 *
 * Projection formula
 * ──────────────────
 * Given a world point (x, y):
 *
 *   isoX = (x - y) * 0.75 + 640     ← Horizontal screen position
 *   isoY = (x + y) * 0.375 + 40     ← Vertical screen position (tilted plane)
 *
 * The constants 0.75 and 0.375 produce a 2:1 isometric ratio (the classic
 * "game isometric" look).  The offsets +640 and +40 centre the map horizontally
 * and provide a top margin.
 *
 * Building height
 * ───────────────
 * To extrude a building upward on screen, shiftUp() moves a screen point
 * in the −Y direction (upward) by `height * 0.6`.  The 0.6 factor controls
 * how tall buildings appear relative to their stored height value.
 */

#pragma once

#include "CityArea.h" // For Vec2.

class Projection2_5D {
public:
    /**
     * @brief Project a world-space 2D point onto the isometric screen plane.
     *
     * @param point  World-space (x, y) coordinate (in scaled pixels).
     * @return       Screen-space (isoX, isoY) position.
     */
    static Vec2 projectPoint(const Vec2& point);

    /**
     * @brief Shift a projected screen point upward to simulate building height.
     *
     * In isometric projection, height is represented by moving a point
     * upward (negative Y in screen space).
     *
     * @param point   Base screen-space position (already projected).
     * @param height  Building extrusion height in world units.
     * @return        Shifted screen-space position (roof level).
     */
    static Vec2 shiftUp(const Vec2& point, float height);
};