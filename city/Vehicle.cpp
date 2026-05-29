#include "Vehicle.h"
#include <cmath>

Vehicle::Vehicle()
    : position(0.0f, 0.0f),
      angleDegrees(0.0f),
      speed(80.0f),
      currentTargetIndex(1),
      routeReady(false) {

    // Simple car shape centered around origin
    localVertices.push_back(Vec2(-18.0f, -10.0f));
    localVertices.push_back(Vec2(18.0f, -10.0f));
    localVertices.push_back(Vec2(18.0f, 10.0f));
    localVertices.push_back(Vec2(-18.0f, 10.0f));

    updateTransform();
}

void Vehicle::setRoute(const std::vector<Vec2>& routePoints) {
    route = routePoints;

    if (route.size() >= 2) {
        position = route[0];
        currentTargetIndex = 1;
        routeReady = true;
    } else {
        routeReady = false;
    }

    updateTransform();
}

void Vehicle::reset() {
    if (route.size() >= 2) {
        position = route[0];
        currentTargetIndex = 1;
        routeReady = true;
    }

    updateTransform();
}

void Vehicle::update(float deltaTime) {
    if (!routeReady || route.size() < 2) {
        return;
    }

    Vec2 target = route[currentTargetIndex];
    Vec2 direction(target.x - position.x, target.y - position.y);

    float dist = distance(position, target);

    if (dist < 5.0f) {
        currentTargetIndex++;

        if (currentTargetIndex >= static_cast<int>(route.size())) {
            currentTargetIndex = 1;
            position = route[0];
        }

        return;
    }

    Vec2 dir = normalize(direction);

    position.x += dir.x * speed * deltaTime;
    position.y += dir.y * speed * deltaTime;

    angleDegrees = std::atan2(dir.y, dir.x) * 180.0f / 3.14159265f;

    updateTransform();
}

void Vehicle::updateTransform() {
    Matrix3x3 scale = Matrix3x3::scaling(1.0f, 1.0f);
    Matrix3x3 rotation = Matrix3x3::rotation(angleDegrees);
    Matrix3x3 translation = Matrix3x3::translation(position.x, position.y);

    // Composite transformation: Translation × Rotation × Scaling
    transformMatrix = translation * rotation * scale;

    transformedVertices.clear();

    for (const Vec2& vertex : localVertices) {
        transformedVertices.push_back(transformMatrix.transformPoint(vertex));
    }
}

float Vehicle::distance(const Vec2& a, const Vec2& b) const {
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    return std::sqrt(dx * dx + dy * dy);
}

Vec2 Vehicle::normalize(const Vec2& v) const {
    float length = std::sqrt(v.x * v.x + v.y * v.y);

    if (length == 0.0f) {
        return Vec2(0.0f, 0.0f);
    }

    return Vec2(v.x / length, v.y / length);
}

const std::vector<Vec2>& Vehicle::getTransformedVertices() const {
    return transformedVertices;
}

Vec2 Vehicle::getPosition() const {
    return position;
}

float Vehicle::getAngle() const {
    return angleDegrees;
}

float Vehicle::getSpeed() const {
    return speed;
}

Matrix3x3 Vehicle::getTransformMatrix() const {
    return transformMatrix;
}