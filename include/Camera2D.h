/**
 * @file Camera2D.h
 * @brief Declares the Camera2D class — a 2D viewport with pan, zoom, and rotation.
 *
 * The camera accumulates three independent transforms that are applied to every
 * world-space point before it is drawn:
 *
 *   1. **Rotation**  — around a configurable pivot (the loaded area's centre).
 *   2. **Scale/Zoom** — uniform scale factor clamped to [0.4 … 3.0].
 *   3. **Translation (pan)** — pixel offset added after scaling.
 *
 * The Renderer calls Camera2D::rotateWorldPoint() and Camera2D::worldToScreen()
 * for every vertex to obtain its final screen-space position.
 */

#pragma once

#include "CityArea.h" // For Vec2.

class Camera2D {
public:
    /** @brief Construct a camera at the origin with zoom = 1 and no rotation. */
    Camera2D();

    // ── Pan (translation) ─────────────────────────────────────────────────────

    /**
     * @brief Shift the viewport by (dx, dy) pixels.
     * @param dx  Horizontal offset (positive = shift scene right).
     * @param dy  Vertical offset   (positive = shift scene down).
     */
    void pan(float dx, float dy);

    // ── Zoom ──────────────────────────────────────────────────────────────────

    /** @brief Increase zoom by 0.05, capped at 3.0. */
    void zoomIn();

    /** @brief Decrease zoom by 0.05, capped at minimum 0.4. */
    void zoomOut();

    // ── Reset ─────────────────────────────────────────────────────────────────

    /** @brief Reset pan, zoom, and rotation to their default values. */
    void reset();

    // ── Coordinate conversion ─────────────────────────────────────────────────

    /**
     * @brief Convert a world-space point to screen-space.
     *
     * screen.x = world.x * zoom + offsetX
     * screen.y = world.y * zoom + offsetY
     *
     * Note: rotation is NOT applied here; callers should first call
     * rotateWorldPoint() if rotation is needed.
     */
    Vec2 worldToScreen(const Vec2& worldPoint) const;

    /**
     * @brief Convert a screen-space point back to world-space.
     *
     * The inverse of worldToScreen (without rotation).
     */
    Vec2 screenToWorld(const Vec2& screenPoint) const;

    // ── Getters ───────────────────────────────────────────────────────────────

    float getOffsetX()   const; ///< Horizontal pan offset (pixels).
    float getOffsetY()   const; ///< Vertical pan offset (pixels).
    float getZoom()      const; ///< Current zoom factor.
    float getRotation()  const; ///< Current rotation angle in radians.

    // ── Rotation ──────────────────────────────────────────────────────────────

    /**
     * @brief Set the world-space point around which rotation is applied.
     *
     * Should be called with the centre of the loaded city area so the city
     * spins around its own centre rather than the world origin.
     */
    void setRotationCenter(const Vec2& center);

    /**
     * @brief Rotate a world-space point around the rotation pivot.
     *
     * Standard 2D rotation:
     *   p' = R * (p - pivot) + pivot
     * where R is the 2D rotation matrix for `rotationAngle`.
     *
     * If rotationAngle == 0, the point is returned unchanged.
     */
    Vec2 rotateWorldPoint(const Vec2& worldPoint) const;

    /**
     * @brief Rotate the camera anti-clockwise.
     * @param deltaTime  Time elapsed (s); rotates at 1.5 rad/s.
     */
    void rotateLeft(float deltaTime);

    /**
     * @brief Rotate the camera clockwise.
     * @param deltaTime  Time elapsed (s); rotates at 1.5 rad/s.
     */
    void rotateRight(float deltaTime);

private:
    float offsetX;          ///< Horizontal pan offset in screen pixels.
    float offsetY;          ///< Vertical pan offset in screen pixels.
    float zoom;             ///< Uniform zoom scale factor.
    float rotationAngle;    ///< Current rotation angle in radians (positive = clockwise).
    Vec2  rotationCenter;   ///< World-space point used as the rotation pivot.
};