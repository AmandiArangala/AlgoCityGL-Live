/**
 * @file SignalController.h
 * @brief Declares SignalController and related types for the traffic-light simulation.
 *
 * Traffic light state machine
 * ────────────────────────────
 * Each signal cycles through three states:
 *
 *   Red ──(redDuration s)──→ Green ──(greenDuration s)──→ Yellow ──(yellowDuration s)──→ Red …
 *
 * The durations are defined in the static TrafficLight struct (loaded from JSON).
 * SignalController wraps each TrafficLight in a RuntimeTrafficLight that adds
 * a live `state` and a running `timer` so the simulation can progress.
 *
 * Integration
 * ────────────
 * - Application::run() calls signalController.update(1/60) each frame while playing.
 * - Vehicle::shouldStopForRedLight() reads each RuntimeTrafficLight's state to
 *   decide whether to halt.
 * - The Renderer reads state to choose the correct colour dot to draw at the signal.
 */

#pragma once

#include <vector>
#include "CityArea.h" // For TrafficLight, CityArea.

// ─────────────────────────────────────────────────────────────────────────────
// SignalState — the three phases of a traffic signal
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief The three possible states of a traffic light.
 *
 * Using an enum class (scoped enum) prevents name collisions and ensures
 * only valid values are used.
 */
enum class SignalState {
    Red,    ///< Vehicles must stop.
    Yellow, ///< Vehicles should prepare to stop (transition phase).
    Green   ///< Vehicles may proceed.
};

// ─────────────────────────────────────────────────────────────────────────────
// RuntimeTrafficLight — live state wrapping a static TrafficLight config
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Combines a static traffic-light configuration with live runtime state.
 *
 * `baseLight`  → immutable config loaded from JSON (position, durations, direction).
 * `state`      → current phase (Red / Yellow / Green).
 * `timer`      → elapsed seconds in the current phase; resets when state changes.
 */
struct RuntimeTrafficLight {
    TrafficLight baseLight;              ///< Static config (position, cycle timings, direction).
    SignalState  state = SignalState::Red; ///< Current phase; starts Red.
    float        timer = 0.0f;           ///< Seconds elapsed in the current phase.
};

// ─────────────────────────────────────────────────────────────────────────────
// SignalController — manages all traffic lights in the loaded area
// ─────────────────────────────────────────────────────────────────────────────

class SignalController {
public:
    /**
     * @brief Create RuntimeTrafficLight entries from the area's static config.
     *
     * Called once when a new area is loaded.  Reads the `initialState` and
     * `initialTimer` fields from each TrafficLight so signals can start
     * at different phases (staggering green waves, etc.).
     *
     * @param area  The newly loaded city area.
     */
    void initializeFromArea(const CityArea& area);

    /**
     * @brief Advance all traffic light timers by deltaTime seconds.
     *
     * Transitions each light to the next state when its timer exceeds the
     * configured duration for the current state.
     *
     * @param deltaTime  Elapsed time in seconds (typically 1/60 per frame).
     */
    void update(float deltaTime);

    /** @brief Reset all lights to Red with timer = 0. */
    void reset();

    /**
     * @brief Return the current list of runtime traffic lights (read-only).
     *
     * Used by the Renderer (to draw signal heads) and by Vehicle
     * (to check whether to stop).
     */
    const std::vector<RuntimeTrafficLight>& getTrafficLights() const;

    /** @brief Returns true if at least one traffic light has been initialized. */
    bool hasTrafficLights() const;

private:
    std::vector<RuntimeTrafficLight> trafficLights; ///< Live state for all signals.

    /**
     * @brief Advance a single traffic light's timer and handle state transitions.
     *
     * @param light      The light to update (modified in place).
     * @param deltaTime  Elapsed time in seconds.
     */
    void updateSingleLight(RuntimeTrafficLight& light, float deltaTime);
};