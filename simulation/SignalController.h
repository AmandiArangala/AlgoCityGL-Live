#pragma once

#include <vector>
#include "CityArea.h"

enum class SignalState {
    Red,
    Yellow,
    Green
};

struct RuntimeTrafficLight {
    TrafficLight baseLight;
    SignalState state = SignalState::Red;
    float timer = 0.0f;
};

class SignalController {
public:
    void initializeFromArea(const CityArea& area);
    void update(float deltaTime);
    void reset();

    const std::vector<RuntimeTrafficLight>& getTrafficLights() const;
    bool hasTrafficLights() const;

private:
    std::vector<RuntimeTrafficLight> trafficLights;

    void updateSingleLight(RuntimeTrafficLight& light, float deltaTime);
};