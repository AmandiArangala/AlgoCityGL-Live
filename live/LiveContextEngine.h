/**
 * @file LiveContextEngine.h
 * @brief Declares LiveContextEngine — the active environment / weather mode.
 *
 * The engine holds a single LiveContextMode enum value and exposes helper
 * predicates (isNightMode(), isRainMode(), etc.) so the Renderer and
 * VehicleController can branch on the current environmental state without
 * comparing enum values directly.
 *
 * Weather effects on simulation:
 *   Sunny       — vehicle speed multiplier 1.0x (normal speed)
 *   Rain        — vehicle speed multiplier 0.65x (slower, wet roads)
 *   Night       — vehicle speed multiplier 1.0x  (night-time lighting only)
 *   HeavyTraffic — vehicle speed multiplier 0.45x (congestion)
 *   Incident    — vehicle speed multiplier 0.55x  (caution zone)
 */
#pragma once

/** @brief Enumerates the five supported environmental / weather modes. */
enum class LiveContextMode {
    Sunny,        ///< Clear daytime — default mode.
    Rain,         ///< Raining — vehicles slow down, rain overlay shown.
    Night,        ///< Night-time — streetlights and window glow activated.
    HeavyTraffic, ///< Congestion scenario — vehicles significantly slowed.
    Incident      ///< Road incident — incident marker drawn, vehicles slow.
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
     * @brief Maps a combo-box integer index (0–4) to a LiveContextMode and stores it.
     * @param selectedMode  Integer index matching the weatherModes[] array in ImGuiPanels.
     */
    void setMode(int selectedMode);

    /** @brief Returns the current LiveContextMode enum value. */
    LiveContextMode getMode() const;

    bool isSunnyMode()        const; ///< Returns true when mode == Sunny.
    bool isRainMode()         const; ///< Returns true when mode == Rain.
    bool isNightMode()        const; ///< Returns true when mode == Night.
    bool isHeavyTrafficMode() const; ///< Returns true when mode == HeavyTraffic.
    bool isIncidentMode()     const; ///< Returns true when mode == Incident.

    /**
     * @brief Returns a speed multiplier applied to all vehicles.
     * @return 0.45 (HeavyTraffic), 0.55 (Incident), 0.65 (Rain), or 1.0 (others).
     */
    float getVehicleSpeedMultiplier() const;

private:
    LiveContextMode mode = LiveContextMode::Sunny; ///< Current active mode.
};