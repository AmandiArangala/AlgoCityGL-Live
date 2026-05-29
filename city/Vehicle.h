#pragma once

#include <vector>
#include "CityArea.h"
#include "Matrix3x3.h"

class Vehicle {
public:
    Vehicle();

    void setRoute(const std::vector<Vec2>& routePoints);
    void update(float deltaTime);
    void reset();

    const std::vector<Vec2>& getTransformedVertices() const;

    Vec2 getPosition() const;
    float getAngle() const;
    float getSpeed() const;
    Matrix3x3 getTransformMatrix() const;

private:
    std::vector<Vec2> route;
    std::vector<Vec2> localVertices;
    std::vector<Vec2> transformedVertices;

    Vec2 position;
    float angleDegrees;
    float speed;

    int currentTargetIndex;
    bool routeReady;

    Matrix3x3 transformMatrix;

    void updateTransform();
    float distance(const Vec2& a, const Vec2& b) const;
    Vec2 normalize(const Vec2& v) const;
};