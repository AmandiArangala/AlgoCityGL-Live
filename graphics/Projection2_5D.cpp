/**
 * @file Projection2_5D.cpp
 * @brief Implements the isometric (2.5D) projection for the city renderer.
 *
 * Isometric projection math
 * ─────────────────────────
 * Classic isometric projection rotates the world 45° and tilts it ~30°.
 * The simplified "dimetric" form used here avoids trigonometry by using
 * pre-computed integer ratios:
 *
 *   isoX = (x - y) * 0.75  +  centreOffsetX
 *   isoY = (x + y) * 0.375 +  topOffsetY
 *
 * Why these constants?
 *  • (x - y): Computes the horizontal diamond axis of the isometric grid.
 *  • (x + y): Computes the vertical diamond axis.
 *  • 0.75    : Horizontal scale (3/4 of the world unit → screen pixel ratio).
 *  • 0.375   : Vertical scale  (3/8), giving the classic 2:1 width-to-height ratio.
 *  • +640    : Centres the projection horizontally for a 1280-wide window.
 *  • +40     : Adds a small top margin so the city doesn't clip the window edge.
 *
 * Building height
 * ───────────────
 * In the rendered 2.5D view, "up" on screen corresponds to higher altitude.
 * shiftUp() simply subtracts from the screen Y coordinate to move a point higher.
 * The factor 0.6 scales the stored building height to a visually appropriate
 * number of screen pixels.
 */

#include "Projection2_5D.h"

/**
 * @brief Project a flat world-space point into isometric screen-space.
 *
 * These constants are chosen to keep the map visible in the current window.
 */
Vec2 Projection2_5D::projectPoint(const Vec2& point) {
    // Horizontal screen position: the difference (x - y) creates the left-right
    // lean of the isometric grid.  Scaled by 0.75 and centred at x=640.
    float isoX = (point.x - point.y) * 0.75f + 640.0f;

    // Vertical screen position: the sum (x + y) creates the top-bottom lean.
    // Scaled by 0.375 (half of 0.75) for the 2:1 isometric ratio, offset by 40.
    float isoY = (point.x + point.y) * 0.375f + 40.0f;

    return Vec2(isoX, isoY);
}

/**
 * @brief Move a projected screen point upward to represent building height.
 *
 * Building height goes upward on screen (negative Y direction in ImGui
 * screen space where Y=0 is the top).
 *
 * @param point   Already-projected base (ground-level) screen position.
 * @param height  World-space height; scaled by 0.6 to convert to screen pixels.
 */
Vec2 Projection2_5D::shiftUp(const Vec2& point, float height) {
    return Vec2(point.x, point.y - height * 0.6f);
}