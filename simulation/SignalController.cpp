/**
 * @file SignalController.cpp
 * @brief Implements the traffic-light state machine for AlgoCityGL Live.
 *
 * State machine diagram (per light):
 *
 *   Red  ──(redDuration s)──> Green  ──(greenDuration s)──> Yellow  ──(yellowDuration s)──> Red ...
 *
 * updateSingleLight() is called every frame for every light.
 * It increments the per-light timer and triggers the appropriate phase
 * transition when the timer exceeds the configured duration.
 */
#include "SignalController.h"

void SignalController::initializeFromArea(const CityArea& area) {
    trafficLights.clear(); // Remove any lights from a previously loaded area.

    // Convert each static TrafficLight (from JSON) into a live RuntimeTrafficLight.
    for (const TrafficLight& light : area.trafficLights) {
        RuntimeTrafficLight runtimeLight;
        runtimeLight.baseLight = light;

        // Restore the starting phase as defined in the JSON file.
        if (light.initialState == "Green") {
            runtimeLight.state = SignalState::Green;
        } else if (light.initialState == "Yellow") {
            runtimeLight.state = SignalState::Yellow;
        } else {
            runtimeLight.state = SignalState::Red; // Default to Red.
        }
        runtimeLight.timer = light.initialTimer; // Resume from the saved timer offset.

        trafficLights.push_back(runtimeLight);
    }
}

void SignalController::update(float deltaTime) {
    // Advance every light independently by the same deltaTime.
    for (RuntimeTrafficLight& light : trafficLights) {
        updateSingleLight(light, deltaTime);
    }
}

void SignalController::reset() {
    // Return every light to Red at timer = 0.
    for (RuntimeTrafficLight& light : trafficLights) {
        light.state = SignalState::Red;
        light.timer = 0.0f;
    }
}

void SignalController::updateSingleLight(RuntimeTrafficLight& light, float deltaTime) {
    light.timer += deltaTime; // Accumulate elapsed time in the current phase.

    if (light.state == SignalState::Red) {
        // Transition Red -> Green when redDuration expires.
        if (light.timer >= light.baseLight.redDuration) {
            light.state = SignalState::Green;
            light.timer = 0.0f;
        }
    } else if (light.state == SignalState::Green) {
        // Transition Green -> Yellow when greenDuration expires.
        if (light.timer >= light.baseLight.greenDuration) {
            light.state = SignalState::Yellow;
            light.timer = 0.0f;
        }
    } else if (light.state == SignalState::Yellow) {
        // Transition Yellow -> Red when yellowDuration expires.
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