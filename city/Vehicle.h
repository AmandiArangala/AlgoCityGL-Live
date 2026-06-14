#pragma once

#include <vector>
#include "CityArea.h"
#include "Matrix3x3.h"
#include "SignalController.h"

class Vehicle {
public:
    enum Type { CAR, BUS, TRUCK, BIKE };

    Vehicle();

    void setRoute(const std::vector<Vec2>& routePoints);
    void update(float deltaTime, const std::vector<RuntimeTrafficLight>& trafficLights, const std::vector<Vehicle>& otherVehicles);
    void reset();

    const std::vector<Vec2>& getTransformedVertices() const;

    Vec2 getPosition() const;
    float getAngle() const;
    float getSpeed() const;
    bool getIsStopped() const;
    int getCurrentTargetIndex() const;
    int getRouteSize() const;
    float getOpacity() const;
    Matrix3x3 getTransformMatrix() const;
    Type getType() const;

private:
    std::vector<Vec2> route;
    std::vector<Vec2> localVertices;
    std::vector<Vec2> transformedVertices;

    Type type;

    Vec2 position;
    float angleDegrees;
    float speed;
    float opacity = 1.0f;

    int currentTargetIndex;
    bool routeReady;
    bool stoppedAtRedLight;

    Matrix3x3 transformMatrix;

    void updateTransform();
    bool shouldStopForRedLight(const std::vector<RuntimeTrafficLight>& trafficLights) const;

    float distance(const Vec2& a, const Vec2& b) const;
    Vec2 normalize(const Vec2& v) const;
};