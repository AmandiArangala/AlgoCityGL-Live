/**
 * @file SignalController.cpp
 * @brief Implements the traffic-light state machine for AlgoCityGL Live.
 *
 * State machine diagram (per light)
 * ───────────────────────────────────
 *
 *   ┌─────────────────────────────────────────────────────┐
 *   │                                                     │
 *   ▼                                                     │
 *  [Red] ──(timer >= redDuration)──→ [Green]              │
 *                                       │                 │
 *                              (timer >= greenDuration)   │
 *                                       │                 │
 *                                       ▼                 │
 *                                   [Yellow] ─(timer >= yellowDuration)─┘
 *
 * Each light independently tracks its own timer and state.
 * The update() method advances every light each frame.
 *
 * Staggered starts
 * ─────────────────
 * The `initialState` and `initialTimer` fields in the JSON let lights start
 * mid-cycle.  For example, setting initialState="Green" and initialTimer=10
 * means the light begins green and already has 10 seconds "on the clock",
 * so it will turn yellow in (greenDuration - 10) seconds.  This prevents all
 * signals in an area from turning green simultaneously.
 */

#include "SignalController.h"

// ─────────────────────────────────────────────────────────────────────────────
// initializeFromArea — Convert static config to live runtime state
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Build RuntimeTrafficLight entries from the area's TrafficLight array.
 *
 * Clears any previous state first so that reloading an area produces a fresh set.
 * Each light's initial phase (Red / Yellow / Green) and elapsed timer are
 * read directly from the JSON via the TrafficLight struct fields.
 */
void SignalController::initializeFromArea(const CityArea& area) {
    trafficLights.clear(); // Discard any lights from a previously loaded area.

    for (const TrafficLight& light : area.trafficLights) {
        RuntimeTrafficLight runtimeLight;
        runtimeLight.baseLight = light; // Copy immutable configuration.

        // Set the starting phase from the JSON "initialState" field.
        if (light.initialState == "Green") {
            runtimeLight.state = SignalState::Green;
        } else if (light.initialState == "Yellow") {
            runtimeLight.state = SignalState::Yellow;
        } else {
            runtimeLight.state = SignalState::Red; // Default to Red for safety.
        }

        // Start the timer at the pre-offset value so lights begin mid-cycle.
        runtimeLight.timer = light.initialTimer;

        trafficLights.push_back(runtimeLight);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// update — Tick all traffic lights forward by deltaTime
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Advance every traffic light's state machine by deltaTime seconds.
 *
 * Called once per frame by Application::run() only while the simulation
 * is in the playing state.
 */
void SignalController::update(float deltaTime) {
    for (RuntimeTrafficLight& light : trafficLights) {
        updateSingleLight(light, deltaTime);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// reset — Return all lights to Red
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Force all lights to Red with a fresh timer of 0. */
void SignalController::reset() {
    for (RuntimeTrafficLight& light : trafficLights) {
        light.state = SignalState::Red;
        light.timer = 0.0f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// updateSingleLight — State machine for one traffic light
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Advance one traffic light's timer and transition when the phase expires.
 *
 * Timer logic:
 *   timer += deltaTime                         (always)
 *   if timer >= phaseDuration → advance state, reset timer to 0
 *
 * State transitions:
 *   Red    → Green   (after redDuration seconds)
 *   Green  → Yellow  (after greenDuration seconds)
 *   Yellow → Red     (after yellowDuration seconds)
 */
void SignalController::updateSingleLight(RuntimeTrafficLight& light, float deltaTime) {
    light.timer += deltaTime; // Accumulate elapsed time in the current phase.

    if (light.state == SignalState::Red) {
        if (light.timer >= light.baseLight.redDuration) {
            light.state = SignalState::Green; // Transition: Red → Green.
            light.timer = 0.0f;              // Reset timer for the Green phase.
        }
    } else if (light.state == SignalState::Green) {
        if (light.timer >= light.baseLight.greenDuration) {
            light.state = SignalState::Yellow; // Transition: Green → Yellow.
            light.timer = 0.0f;
        }
    } else if (light.state == SignalState::Yellow) {
        if (light.timer >= light.baseLight.yellowDuration) {
            light.state = SignalState::Red;   // Transition: Yellow → Red.
            light.timer = 0.0f;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Return all runtime traffic lights (read-only reference).
 *
 * The Renderer uses this list to draw the signal heads, and Vehicle::update()
 * uses it to check whether to stop at a red light.
 */
const std::vector<RuntimeTrafficLight>& SignalController::getTrafficLights() const {
    return trafficLights;
}

/** @brief Return true if this controller has at least one traffic light. */
bool SignalController::hasTrafficLights() const {
    return !trafficLights.empty();
}