/**
 * @file SignalController.h
 * @brief Declares SignalController and related types for the traffic-light simulation.
 *
 * Traffic light state machine (per light):
 *
 *   Red  ──(redDuration)──>  Green  ──(greenDuration)──>  Yellow  ──(yellowDuration)──>  Red
 *
 * Each light has independent phase durations read from the JSON file.
 * The timer is reset to 0 whenever the state transitions.
 */
#pragma once

#include <vector>
#include "CityArea.h"

/** @brief Enumerates the three possible traffic-light phases. */
enum class SignalState {
    Red,    ///< Vehicles must stop.
    Yellow, ///< Vehicles should prepare to stop.
    Green   ///< Vehicles may proceed.
};

/**
 * @brief Runtime state of a single traffic light (phase + elapsed timer).
 *
 * Combines the static JSON data (baseLight) with mutable simulation state
 * (state and timer).  SignalController owns a vector of these.
 */
struct RuntimeTrafficLight {
    TrafficLight baseLight;                    ///< Static data loaded from JSON.
    SignalState  state = SignalState::Red;     ///< Current phase of this light.
    float        timer = 0.0f;                 ///< Seconds elapsed in the current phase.
};

/**
 * @brief Manages the lifecycle of all traffic lights in the loaded area.
 *
 * Responsibilities:
 *  - initializeFromArea(): converts static TrafficLight JSON data into
 *    live RuntimeTrafficLight objects with starting states.
 *  - update(dt): advances all timers and triggers phase transitions.
 *  - reset(): resets all lights to Red at timer=0.
 */
class SignalController {
public:
    /**
     * @brief Reads trafficLights from the loaded area and builds RuntimeTrafficLight objects.
     * @param area  The currently loaded CityArea.
     */
    void initializeFromArea(const CityArea& area);

    /**
     * @brief Advances every traffic light by deltaTime seconds.
     * @param deltaTime  Elapsed time since last frame (usually 1/60 s).
     */
    void update(float deltaTime);

    /** @brief Resets all lights to Red with timer = 0. */
    void reset();

    /** @brief Returns a const reference to all runtime traffic lights for rendering. */
    const std::vector<RuntimeTrafficLight>& getTrafficLights() const;

    /** @brief Returns true if at least one traffic light is loaded. */
    bool hasTrafficLights() const;

private:
    std::vector<RuntimeTrafficLight> trafficLights; ///< Live state for every light.

    /** @brief Advances a single light through its Red → Green → Yellow → Red cycle. */
    void updateSingleLight(RuntimeTrafficLight& light, float deltaTime);
};