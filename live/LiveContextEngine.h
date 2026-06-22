/**
 * @file LiveContextEngine.h
 * @brief Declares LiveContextEngine — the active environment / weather mode.
 *
 * The LiveContextEngine is a lightweight state machine with a single field:
 * the current LiveContextMode.  It acts as a central source of truth that
 * multiple subsystems read to change their behaviour:
 *
 *  Subsystem             | What changes
 *  ─────────────────────────────────────────────────────────────
 *  Renderer              | Sky colour, rain particles, night darkness,
 *                        | incident marker, heavy-traffic tint
 *  VehicleController     | deltaTime × getVehicleSpeedMultiplier()
 *
 * Mode effects
 * ─────────────
 *  Mode          | Speed multiplier | Visual effect
 *  ─────────────────────────────────────────────────────────────────────
 *  Sunny         | 1.00  (normal)   | Clear sky, no overlay
 *  Rain          | 0.65  (−35%)     | Blue rain-drop particles
 *  Night         | 1.00  (normal)   | Dark sky, street-light glow
 *  HeavyTraffic  | 0.45  (−55%)     | Orange tint overlay
 *  Incident      | 0.55  (−45%)     | Red blinking incident marker
 *
 * The mode is set each frame by Application::run() from the value
 * selected in the ImGuiPanels weather combo box.
 */

#pragma once

/**
 * @brief Enumeration of all selectable environment / weather modes.
 *
 * Defined as a scoped enum class to prevent name collisions with
 * similarly named constants in other files.
 */
enum class LiveContextMode {
    Sunny,        ///< Normal, clear weather — no visual effects.
    Rain,         ///< Rainfall simulation — speed reduced, rain particles drawn.
    Night,        ///< Night-time — dark sky, street-light glow overlays.
    HeavyTraffic, ///< Traffic jam — orange tint, significantly reduced speed.
    Incident      ///< Road incident — red marker, moderately reduced speed.
};

/**
 * @brief A lightweight state machine that tracks the current environmental mode.
 *
 * setMode() is called every frame from Application::run() with the integer
 * index from ImGuiPanels, keeping the engine in sync with the UI.
 */
class LiveContextEngine {
public:
    /**
     * @brief Set the active mode from a UI combo-box index.
     *
     * The index mapping matches the order in ImGuiPanels::weatherModes[]:
     *   0 → Sunny, 1 → Rain, 2 → Night, 3 → HeavyTraffic, 4 → Incident.
     *
     * @param selectedMode  Integer index from the ImGui combo box.
     */
    void setMode(int selectedMode);

    /** @brief Return the currently active LiveContextMode enum value. */
    LiveContextMode getMode() const;

    // ── Mode queries ──────────────────────────────────────────────────────────
    // Convenience predicates used by the Renderer to branch rendering code.

    bool isSunnyMode()        const; ///< True when mode == Sunny.
    bool isRainMode()         const; ///< True when mode == Rain.
    bool isNightMode()        const; ///< True when mode == Night.
    bool isHeavyTrafficMode() const; ///< True when mode == HeavyTraffic.
    bool isIncidentMode()     const; ///< True when mode == Incident.

    /**
     * @brief Return the speed multiplier for vehicles in the current mode.
     *
     * The VehicleController multiplies deltaTime by this value before passing
     * it to Vehicle::update(), effectively slowing or maintaining vehicle speed:
     *
     *   Sunny        → 1.00 (no change)
     *   Rain         → 0.65 (35% slower)
     *   Night        → 1.00 (no change)
     *   HeavyTraffic → 0.45 (55% slower)
     *   Incident     → 0.55 (45% slower)
     */
    float getVehicleSpeedMultiplier() const;

private:
    LiveContextMode mode = LiveContextMode::Sunny; ///< Current active mode; defaults to Sunny.
};