#pragma once

#include "CityArea.h"

class Camera2D {
public:
    Camera2D();

    void pan(float dx, float dy);
    void zoomIn();
    void zoomOut();
    void reset();

    Vec2 worldToScreen(const Vec2& worldPoint) const;
    Vec2 screenToWorld(const Vec2& screenPoint) const;

    float getOffsetX() const;
    float getOffsetY() const;
    float getZoom() const;

private:
    float offsetX;
    float offsetY;
    float zoom;
};