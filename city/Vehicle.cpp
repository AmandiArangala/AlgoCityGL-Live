#include "Vehicle.h"
#include <cmath>
#include <cstdlib>

Vehicle::Vehicle()
    : position(0.0f, 0.0f),
      angleDegrees(0.0f),
      speed(80.0f),
      currentTargetIndex(1),
      routeReady(false),
      stoppedAtRedLight(false) {

    type = static_cast<Type>(std::rand() % 4);

    if (type == CAR) {
        localVertices.push_back(Vec2(-18.0f, -10.0f));
        localVertices.push_back(Vec2(18.0f, -10.0f));
        localVertices.push_back(Vec2(18.0f, 10.0f));
        localVertices.push_back(Vec2(-18.0f, 10.0f));
        speed = 80.0f + (std::rand() % 20);
    } else if (type == BUS) {
        localVertices.push_back(Vec2(-28.0f, -11.0f));
        localVertices.push_back(Vec2(28.0f, -11.0f));
        localVertices.push_back(Vec2(28.0f, 11.0f));
        localVertices.push_back(Vec2(-28.0f, 11.0f));
        speed = 50.0f + (std::rand() % 15);
    } else if (type == TRUCK) {
        localVertices.push_back(Vec2(-24.0f, -12.0f));
        localVertices.push_back(Vec2(24.0f, -12.0f));
        localVertices.push_back(Vec2(24.0f, 12.0f));
        localVertices.push_back(Vec2(-24.0f, 12.0f));
        speed = 55.0f + (std::rand() % 15);
    } else if (type == BIKE) {
        localVertices.push_back(Vec2(-8.0f, -4.0f));
        localVertices.push_back(Vec2(8.0f, -4.0f));
        localVertices.push_back(Vec2(8.0f, 4.0f));
        localVertices.push_back(Vec2(-8.0f, 4.0f));
        speed = 70.0f + (std::rand() % 20);
    }

    updateTransform();
}

void Vehicle::setRoute(const std::vector<Vec2>& routePoints) {
    route = routePoints;

    if (route.size() >= 2) {
        position = route[0];
        currentTargetIndex = 1;
        routeReady = true;
        stoppedAtRedLight = false;

        Vec2 direction(route[1].x - route[0].x, route[1].y - route[0].y);

        if (distance(route[0], route[1]) > 0.0f) {
            angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
        }
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
        stoppedAtRedLight = false;

        Vec2 direction(route[1].x - route[0].x, route[1].y - route[0].y);
        angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
    }

    updateTransform();
}

void Vehicle::update(float deltaTime, const std::vector<RuntimeTrafficLight>& trafficLights, const std::vector<Vehicle>& otherVehicles) {
    if (!routeReady || route.size() < 2) {
        return;
    }

    if (shouldStopForRedLight(trafficLights)) {
        stoppedAtRedLight = true;
        updateTransform();
        return;
    }

    // Collision avoidance with other vehicles ahead
    bool vehicleAhead = false;
    float safeDistance = 70.0f; // Gap to maintain
    
    Vec2 direction = (currentTargetIndex < route.size()) 
        ? Vec2(route[currentTargetIndex].x - position.x, route[currentTargetIndex].y - position.y)
        : Vec2(0.0f, 0.0f);
    Vec2 forward = normalize(direction);

    for (const Vehicle& other : otherVehicles) {
        if (&other == this) continue;

        float d = distance(position, other.getPosition());
        if (d < safeDistance) {
            Vec2 toOther(other.getPosition().x - position.x, other.getPosition().y - position.y);
            Vec2 toOtherDir = normalize(toOther);
            
            // Check if the other vehicle is in front
            float dotProduct = forward.x * toOtherDir.x + forward.y * toOtherDir.y;
            if (dotProduct > 0.8f) {
                // Ensure they are roughly moving in same direction (not oncoming traffic crossing paths)
                Vec2 otherForward(std::cos(other.getAngle() * 3.14159f / 180.0f), std::sin(other.getAngle() * 3.14159f / 180.0f));
                if (forward.x * otherForward.x + forward.y * otherForward.y > 0.5f) {
                    vehicleAhead = true;
                    break;
                }
            }
        }
    }

    if (vehicleAhead) {
        stoppedAtRedLight = true; // reusing visual state for being stopped
        updateTransform();
        return;
    }

    stoppedAtRedLight = false;

    float moveDist = speed * deltaTime;

    while (moveDist > 0.0f) {
        if (currentTargetIndex >= static_cast<int>(route.size())) {
            currentTargetIndex = 1;
            position = route[0];
            opacity = 1.0f;
            break;
        }

        Vec2 target = route[currentTargetIndex];
        float dist = distance(position, target);

        if (dist > 0.001f) {
            Vec2 direction(target.x - position.x, target.y - position.y);
            angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
        }

        if (moveDist >= dist) {
            position = target;
            moveDist -= dist;
            currentTargetIndex++;
        } else {
            Vec2 direction(target.x - position.x, target.y - position.y);
            Vec2 dir = normalize(direction);
            position.x += dir.x * moveDist;
            position.y += dir.y * moveDist;
            moveDist = 0.0f;
        }
    }

    opacity = 1.0f;

    updateTransform();
}

bool Vehicle::shouldStopForRedLight(const std::vector<RuntimeTrafficLight>& trafficLights) const {
    if (route.empty() || currentTargetIndex >= route.size()) {
        return false;
    }

    Vec2 direction(route[currentTargetIndex].x - position.x, route[currentTargetIndex].y - position.y);
    Vec2 forward = normalize(direction);

    for (const RuntimeTrafficLight& light : trafficLights) {
        float d = distance(position, light.baseLight.position);

        bool nearLight = d < 60.0f;

        if (nearLight && light.state == SignalState::Red) {
            // Check if the light governs this vehicle's direction
            bool governsDirection = false;
            if (light.baseLight.direction.x == 0.0f && light.baseLight.direction.y == 0.0f) {
                // Backward compatibility: no direction means governs all
                governsDirection = true;
            } else {
                float lightDotProduct = forward.x * light.baseLight.direction.x + forward.y * light.baseLight.direction.y;
                if (lightDotProduct > 0.8f) {
                    governsDirection = true;
                }
            }

            if (governsDirection) {
                Vec2 toLight(light.baseLight.position.x - position.x, light.baseLight.position.y - position.y);
                Vec2 toLightDir = normalize(toLight);
                
                float dotProduct = forward.x * toLightDir.x + forward.y * toLightDir.y;
                
                if (dotProduct > 0.5f) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Vehicle::updateTransform() {
    Matrix3x3 scale = Matrix3x3::scaling(1.0f, 1.0f);
    Matrix3x3 rotation = Matrix3x3::rotation(angleDegrees);
    
    // Offset vehicles to the left lane (approx 16 units)
    Matrix3x3 laneOffset = Matrix3x3::translation(0.0f, -16.0f);

    Matrix3x3 translation = Matrix3x3::translation(position.x, position.y);

    transformMatrix = translation * rotation * laneOffset * scale;

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
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len > 0.0f) {
        return Vec2(v.x / len, v.y / len);
    }
    return Vec2(0.0f, 0.0f);
}

Vehicle::Type Vehicle::getType() const {
    return type;
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

bool Vehicle::getIsStopped() const {
    return stoppedAtRedLight;
}

int Vehicle::getCurrentTargetIndex() const {
    return currentTargetIndex;
}

int Vehicle::getRouteSize() const {
    return static_cast<int>(route.size());
}

float Vehicle::getOpacity() const {
    return opacity;
}

Matrix3x3 Vehicle::getTransformMatrix() const {
    return transformMatrix;
}