/**
 * @file LiveContextEngine.cpp
 * @brief Implements LiveContextEngine — mode switching and speed multiplier lookup.
 *
 * Design
 * ──────
 * The engine stores a single enum field (mode) and exposes it via a small
 * set of accessor methods.  All rendering and simulation decisions based on
 * the mode are made by the callers (Renderer, VehicleController) rather than
 * inside this class, keeping LiveContextEngine focused on a single responsibility:
 * "what mode is currently active?"
 *
 * Speed multipliers
 * ──────────────────
 * The multipliers were chosen to create a noticeable but not extreme difference
 * between modes:
 *   Rain         → 0.65  (wet roads, cautious driving)
 *   Incident     → 0.55  (traffic slowing around an obstruction)
 *   HeavyTraffic → 0.45  (stop-and-go congestion)
 * Sunny and Night use 1.0 (unaffected).
 */

#include "LiveContextEngine.h"

// ─────────────────────────────────────────────────────────────────────────────
// setMode — Map a UI combo-box index to a LiveContextMode
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Set the active mode using the integer index from the ImGui combo box.
 *
 * Index values match the order of entries in ImGuiPanels::weatherModes[]:
 *   0 → Sunny   1 → Rain   2 → Night   3 → HeavyTraffic   4 → Incident
 *
 * Any unrecognised index defaults to Sunny.
 */
void LiveContextEngine::setMode(int selectedMode) {
    // Map the ImGuiPanels combo-box integer index to the corresponding enum.
    switch (selectedMode) {
        case 0: mode = LiveContextMode::Sunny;        break;
        case 1: mode = LiveContextMode::Rain;         break;
        case 2: mode = LiveContextMode::Night;        break;
        case 3: mode = LiveContextMode::HeavyTraffic; break;
        case 4: mode = LiveContextMode::Incident;     break;
        default: mode = LiveContextMode::Sunny;       break; // Safe fallback.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Return the current LiveContextMode enum value. */
LiveContextMode LiveContextEngine::getMode() const {
    return mode;
}

// ── Mode predicate helpers ─────────────────────────────────────────────────
// Each returns true only when that specific mode is active.

bool LiveContextEngine::isSunnyMode()        const { return mode == LiveContextMode::Sunny;        }
bool LiveContextEngine::isRainMode()         const { return mode == LiveContextMode::Rain;         }
bool LiveContextEngine::isNightMode()        const { return mode == LiveContextMode::Night;        }
bool LiveContextEngine::isHeavyTrafficMode() const { return mode == LiveContextMode::HeavyTraffic; }
bool LiveContextEngine::isIncidentMode()     const { return mode == LiveContextMode::Incident;     }

// ─────────────────────────────────────────────────────────────────────────────
// getVehicleSpeedMultiplier — Return the speed factor for the active mode
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Return a multiplier that scales vehicle speed for the current mode.
 *
 * Applied by VehicleController:
 *   effectiveDeltaTime = (1/60) × getVehicleSpeedMultiplier()
 *
 * Modes that return 1.0 have no effect on vehicle speed.
 */
float LiveContextEngine::getVehicleSpeedMultiplier() const {
    // Rain slows vehicles: wet roads reduce traction.
    if (mode == LiveContextMode::Rain) {
        return 0.65f; // Wet roads → cautious driving (35% slower).
    }

    // Heavy traffic: vehicles move significantly slower due to congestion.
    if (mode == LiveContextMode::HeavyTraffic) {
        return 0.45f; // Stop-and-go congestion (55% slower).
    }

    // Incident zone: vehicles slow down as they pass the incident marker.
    if (mode == LiveContextMode::Incident) {
        return 0.55f; // Traffic slowing past an obstruction (45% slower).
    }

    return 1.0f; // Sunny and Night: no speed penalty.
}