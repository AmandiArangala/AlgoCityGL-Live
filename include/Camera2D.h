/**
 * @file Camera2D.h
 * @brief Declares the Camera2D class — a 2D viewport with pan, zoom, and rotation.
 *
 * The camera accumulates three independent transforms applied to every world-space
 * point before it is drawn:
 *
 *   1. **Rotation** around a configurable pivot (rotationCenter) in world space.
 *   2. **Zoom**  — a uniform scale factor applied to world coordinates.
 *   3. **Pan**   — a screen-space offset (offsetX, offsetY) added after scaling.
 *
 * Combined in worldToScreen():
 *   screenX = rotatedWorld.x * zoom + offsetX
 *   screenY = rotatedWorld.y * zoom + offsetY
 *
 * Keyboard bindings (handled in Application::processInput):
 *   W/S = pan up/down,  A/D = pan left/right
 *   Q/E = zoom out/in,  Z/X = rotate left/right,  C = reset
 */
#pragma once

#include "CityArea.h"

class Camera2D {
public:
    Camera2D(); ///< Initialises all transforms to identity (no pan, no zoom, no rotation).

    /** @brief Shifts the viewport by (dx, dy) in screen pixels. */
    void pan(float dx, float dy);

    /** @brief Increases zoom by 0.05 up to a maximum of 3.0. */
    void zoomIn();

    /** @brief Decreases zoom by 0.05 down to a minimum of 0.4. */
    void zoomOut();

    /** @brief Resets pan, zoom, and rotation to their initial values. */
    void reset();

    /**
     * @brief Converts a world-space point to screen (pixel) coordinates.
     * @note Does NOT apply rotation — call rotateWorldPoint() first if needed.
     */
    Vec2 worldToScreen(const Vec2& worldPoint) const;

    /** @brief Inverse of worldToScreen(): converts a pixel position back to world space. */
    Vec2 screenToWorld(const Vec2& screenPoint) const;

    float getOffsetX()   const; ///< Returns the horizontal pan offset.
    float getOffsetY()   const; ///< Returns the vertical pan offset.
    float getZoom()      const; ///< Returns the current zoom factor.
    float getRotation()  const; ///< Returns the current rotation angle in radians.

    /** @brief Sets the world-space pivot point that rotation turns around. */
    void setRotationCenter(const Vec2& center);

    /**
     * @brief Rotates a single world-space point around rotationCenter by rotationAngle.
     * @return The rotated point, still in world space.
     */
    Vec2 rotateWorldPoint(const Vec2& worldPoint) const;

    /** @brief Decrements rotationAngle (counter-clockwise rotation at 1.5 rad/s). */
    void rotateLeft(float deltaTime);

    /** @brief Increments rotationAngle (clockwise rotation at 1.5 rad/s). */
    void rotateRight(float deltaTime);

private:
    float offsetX;        ///< Horizontal pan offset in screen pixels.
    float offsetY;        ///< Vertical pan offset in screen pixels.
    float zoom;           ///< Uniform scale factor (1.0 = no zoom).
    float rotationAngle;  ///< Rotation angle in radians (positive = clockwise on screen).
    Vec2 rotationCenter;  ///< World-space point around which rotation pivots.
};