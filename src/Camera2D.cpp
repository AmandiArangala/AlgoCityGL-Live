/**
 * @file Camera2D.cpp
 * @brief Implements Camera2D — pan, zoom, 2D rotation, and coordinate conversion.
 *
 * Coordinate system reminder
 * ──────────────────────────
 * Screen-space in ImGui has (0,0) at the top-left corner with Y increasing
 * downward.  Because of this, "panning up" means adding a positive Y offset
 * (shifting all world points downward on screen).
 *
 * Rotation math
 * ─────────────
 * To rotate point P around pivot C by angle θ:
 *   1.  Translate to origin:  p = P - C
 *   2.  Apply rotation matrix:  p' = (p.x·cos θ - p.y·sin θ,
 *                                     p.x·sin θ + p.y·cos θ)
 *   3.  Translate back:  P' = p' + C
 */

#include "Camera2D.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

Camera2D::Camera2D()
    : offsetX(0.0f), offsetY(0.0f), zoom(1.0f), rotationAngle(0.0f), rotationCenter({0.0f, 0.0f}) {}

// ─────────────────────────────────────────────────────────────────────────────
// Pan
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Move the viewport by (dx, dy) pixels.
 *
 * The offset is added to every point's screen-space position in worldToScreen().
 * A positive dx shifts the entire scene right; positive dy shifts it down.
 */
void Camera2D::pan(float dx, float dy) {
    // Accumulate panning offset (typically driven by WASD keys).
    offsetX += dx;
    offsetY += dy;
}

// ─────────────────────────────────────────────────────────────────────────────
// Zoom
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Zoom in by a fixed step; capped at maximum 3× magnification. */
void Camera2D::zoomIn() {
    zoom += 0.05f;
    if (zoom > 3.0f) zoom = 3.0f; // Prevent excessive magnification.
}

/** @brief Zoom out by a fixed step; capped at minimum 0.4× to keep the city visible. */
void Camera2D::zoomOut() {
    zoom -= 0.05f;
    if (zoom < 0.4f) zoom = 0.4f; // Prevent zooming out to nothing.
}

// ─────────────────────────────────────────────────────────────────────────────
// Reset
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Restore camera to default: no pan, zoom = 1, no rotation. */
void Camera2D::reset() {
    offsetX       = 0.0f;
    offsetY       = 0.0f;
    zoom          = 1.0f;
    rotationAngle = 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate Conversion
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Map a world-space point to its final screen position.
 *
 * The formula is:
 *   screen.x = world.x * zoom + offsetX
 *   screen.y = world.y * zoom + offsetY
 *
 * Rotation must be applied to the world point *before* calling this function
 * (via rotateWorldPoint).
 */
Vec2 Camera2D::worldToScreen(const Vec2& worldPoint) const {
    // Apply zoom first, then translate by the pan offset.
    // Note: Rotation is handled separately via rotateWorldPoint().
    return Vec2(
        worldPoint.x * zoom + offsetX,
        worldPoint.y * zoom + offsetY
    );
}

/**
 * @brief Map a screen-space point back to world coordinates.
 *
 * The inverse of worldToScreen:
 *   world.x = (screen.x - offsetX) / zoom
 *   world.y = (screen.y - offsetY) / zoom
 */
Vec2 Camera2D::screenToWorld(const Vec2& screenPoint) const {
    // Inverse transform: subtract pan offset, then divide by zoom.
    return Vec2(
        (screenPoint.x - offsetX) / zoom,
        (screenPoint.y - offsetY) / zoom
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

float Camera2D::getOffsetX() const { return offsetX; }
float Camera2D::getOffsetY() const { return offsetY; }
float Camera2D::getZoom()    const { return zoom;     }
float Camera2D::getRotation() const { return rotationAngle; }

// ─────────────────────────────────────────────────────────────────────────────
// Rotation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Store the world-space pivot for subsequent rotateWorldPoint() calls.
 *
 * The Application sets this to the bounding-box centre of the loaded area
 * so the city rotates around its own middle.
 */
void Camera2D::setRotationCenter(const Vec2& center) {
    rotationCenter = center;
}

/**
 * @brief Rotate a world-space point around rotationCenter by rotationAngle.
 *
 * Steps:
 *  1. Translate the point so that the pivot becomes the origin.
 *  2. Apply the 2D rotation matrix.
 *  3. Translate back by adding the pivot.
 *
 * Returns the original point unchanged if rotationAngle is zero (fast path).
 */
Vec2 Camera2D::rotateWorldPoint(const Vec2& worldPoint) const {
    if (rotationAngle == 0.0f) return worldPoint; // Fast path: no rotation.

    float s = std::sin(rotationAngle);
    float c = std::cos(rotationAngle);

    // Translate point back to origin (relative to pivot).
    float px = worldPoint.x - rotationCenter.x;
    float py = worldPoint.y - rotationCenter.y;

    // Apply 2D rotation matrix:
    //  [ c  -s ] [ px ]   [ px*c - py*s ]
    //  [ s   c ] [ py ] = [ px*s + py*c ]
    float xNew = px * c - py * s;
    float yNew = px * s + py * c;

    // Translate point back to world space.
    return Vec2(xNew + rotationCenter.x, yNew + rotationCenter.y);
}

/**
 * @brief Rotate the camera anti-clockwise (decreases the angle in radians).
 * @param deltaTime  Elapsed time in seconds; rotation rate = 1.5 rad/s.
 */
void Camera2D::rotateLeft(float deltaTime) {
    rotationAngle -= 1.5f * deltaTime;
}

/**
 * @brief Rotate the camera clockwise (increases the angle in radians).
 * @param deltaTime  Elapsed time in seconds; rotation rate = 1.5 rad/s.
 */
void Camera2D::rotateRight(float deltaTime) {
    rotationAngle += 1.5f * deltaTime;
}