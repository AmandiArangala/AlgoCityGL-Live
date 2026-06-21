/**
 * @file Camera2D.cpp
 * @brief Implements the 2D camera system for viewport manipulation.
 *
 * The camera manages pan, zoom, and rotation state.
 * World coordinates are mapped to screen coordinates using the formula:
 *   screen = (world_rotated * zoom) + offset
 *
 * Rotation is performed around a configurable pivot point (usually the screen centre
 * converted back to world space).
 */
#include "Camera2D.h"
#include <cmath>

Camera2D::Camera2D()
    : offsetX(0.0f), offsetY(0.0f), zoom(1.0f), rotationAngle(0.0f), rotationCenter({0.0f, 0.0f}) {}

void Camera2D::pan(float dx, float dy) {
    // Accumulate panning offset (typically driven by WASD keys).
    offsetX += dx;
    offsetY += dy;
}

void Camera2D::zoomIn() {
    zoom += 0.05f;
    if (zoom > 3.0f) zoom = 3.0f; // Clamp maximum zoom to 3.0x
}

void Camera2D::zoomOut() {
    zoom -= 0.05f;
    if (zoom < 0.4f) zoom = 0.4f; // Clamp minimum zoom to 0.4x (prevent inverting)
}

void Camera2D::reset() {
    offsetX = 0.0f;
    offsetY = 0.0f;
    zoom = 1.0f;
    rotationAngle = 0.0f;
}

Vec2 Camera2D::worldToScreen(const Vec2& worldPoint) const {
    // Apply zoom first, then translate by the pan offset.
    // Note: Rotation is handled separately via rotateWorldPoint().
    return Vec2(
        worldPoint.x * zoom + offsetX,
        worldPoint.y * zoom + offsetY
    );
}

Vec2 Camera2D::screenToWorld(const Vec2& screenPoint) const {
    // Inverse transform: subtract pan offset, then divide by zoom.
    return Vec2(
        (screenPoint.x - offsetX) / zoom,
        (screenPoint.y - offsetY) / zoom
    );
}

float Camera2D::getOffsetX() const { return offsetX; }
float Camera2D::getOffsetY() const { return offsetY; }
float Camera2D::getZoom() const { return zoom; }
float Camera2D::getRotation() const { return rotationAngle; }

void Camera2D::setRotationCenter(const Vec2& center) {
    rotationCenter = center;
}

Vec2 Camera2D::rotateWorldPoint(const Vec2& worldPoint) const {
    if (rotationAngle == 0.0f) return worldPoint; // Fast path for unrotated camera.
    
    float s = std::sin(rotationAngle);
    float c = std::cos(rotationAngle);
    
    // Step 1: Translate point so that the rotation pivot is at the origin.
    float px = worldPoint.x - rotationCenter.x;
    float py = worldPoint.y - rotationCenter.y;
    
    // Step 2: Apply 2D rotation matrix.
    float xNew = px * c - py * s;
    float yNew = px * s + py * c;
    
    // Step 3: Translate point back to world space.
    return Vec2(xNew + rotationCenter.x, yNew + rotationCenter.y);
}
void Camera2D::rotateLeft(float deltaTime) {
    rotationAngle -= 1.5f * deltaTime;
}
void Camera2D::rotateRight(float deltaTime) {
    rotationAngle += 1.5f * deltaTime;
}