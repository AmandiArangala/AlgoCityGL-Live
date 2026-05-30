#include "Camera2D.h"

Camera2D::Camera2D()
    : offsetX(0.0f), offsetY(0.0f), zoom(1.0f) {}

void Camera2D::pan(float dx, float dy) {
    offsetX += dx;
    offsetY += dy;
}

void Camera2D::zoomIn() {
    zoom += 0.05f;

    if (zoom > 3.0f) {
        zoom = 3.0f;
    }
}

void Camera2D::zoomOut() {
    zoom -= 0.05f;

    if (zoom < 0.4f) {
        zoom = 0.4f;
    }
}

void Camera2D::reset() {
    offsetX = 0.0f;
    offsetY = 0.0f;
    zoom = 1.0f;
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

float Camera2D::getOffsetX() const {
    return offsetX;
}

float Camera2D::getOffsetY() const {
    return offsetY;
}

float Camera2D::getZoom() const {
    return zoom;
}