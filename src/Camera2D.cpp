#include "Camera2D.h"
#include <cmath>

Camera2D::Camera2D()
    : offsetX(0.0f), offsetY(0.0f), zoom(1.0f), rotationAngle(0.0f), rotationCenter({0.0f, 0.0f}) {}

void Camera2D::pan(float dx, float dy) {
    offsetX += dx;
    offsetY += dy;
}

void Camera2D::zoomIn() {
    zoom += 0.05f;
    if (zoom > 3.0f) zoom = 3.0f;
}

void Camera2D::zoomOut() {
    zoom -= 0.05f;
    if (zoom < 0.4f) zoom = 0.4f;
}

void Camera2D::reset() {
    offsetX = 0.0f;
    offsetY = 0.0f;
    zoom = 1.0f;
    rotationAngle = 0.0f;
}

Vec2 Camera2D::worldToScreen(const Vec2& worldPoint) const {
    return Vec2(
        worldPoint.x * zoom + offsetX,
        worldPoint.y * zoom + offsetY
    );
}

Vec2 Camera2D::screenToWorld(const Vec2& screenPoint) const {
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
    if (rotationAngle == 0.0f) return worldPoint;
    
    float s = std::sin(rotationAngle);
    float c = std::cos(rotationAngle);
    
    // Translate point back to origin
    float px = worldPoint.x - rotationCenter.x;
    float py = worldPoint.y - rotationCenter.y;
    
    // Rotate point
    float xNew = px * c - py * s;
    float yNew = px * s + py * c;
    
    // Translate point back
    return Vec2(xNew + rotationCenter.x, yNew + rotationCenter.y);
}
void Camera2D::rotateLeft(float deltaTime) {
    rotationAngle -= 1.5f * deltaTime;
}
void Camera2D::rotateRight(float deltaTime) {
    rotationAngle += 1.5f * deltaTime;
}