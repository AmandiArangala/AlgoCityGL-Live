#include "SignalController.h"

void SignalController::initializeFromArea(const CityArea& area) {
    trafficLights.clear();

    for (const TrafficLight& light : area.trafficLights) {
        RuntimeTrafficLight runtimeLight;
        runtimeLight.baseLight = light;
        if (light.initialState == "Green") {
            runtimeLight.state = SignalState::Green;
        } else if (light.initialState == "Yellow") {
            runtimeLight.state = SignalState::Yellow;
        } else {
            runtimeLight.state = SignalState::Red;
        }
        runtimeLight.timer = light.initialTimer;

        trafficLights.push_back(runtimeLight);
    }
}

void SignalController::update(float deltaTime) {
    for (RuntimeTrafficLight& light : trafficLights) {
        updateSingleLight(light, deltaTime);
    }
}

void SignalController::reset() {
    for (RuntimeTrafficLight& light : trafficLights) {
        light.state = SignalState::Red;
        light.timer = 0.0f;
    }
}

void SignalController::updateSingleLight(RuntimeTrafficLight& light, float deltaTime) {
    light.timer += deltaTime;

    if (light.state == SignalState::Red) {
        if (light.timer >= light.baseLight.redDuration) {
            light.state = SignalState::Green;
            light.timer = 0.0f;
        }
    } else if (light.state == SignalState::Green) {
        if (light.timer >= light.baseLight.greenDuration) {
            light.state = SignalState::Yellow;
            light.timer = 0.0f;
        }
    } else if (light.state == SignalState::Yellow) {
        if (light.timer >= light.baseLight.yellowDuration) {
            light.state = SignalState::Red;
            light.timer = 0.0f;
        }
    }
}

const std::vector<RuntimeTrafficLight>& SignalController::getTrafficLights() const {
    return trafficLights;
}

bool SignalController::hasTrafficLights() const {
    return !trafficLights.empty();
}