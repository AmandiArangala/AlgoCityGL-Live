#pragma once

#include "CityArea.h"

class Projection2_5D {
public:
    static Vec2 projectPoint(const Vec2& point);
    static Vec2 shiftUp(const Vec2& point, float height);
};