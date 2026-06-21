/**
 * @file LiveContextEngine.cpp
 * @brief Implements LiveContextEngine — mode switching and speed multiplier lookup.
 *
 * setMode() maps the integer combo-box index (0-4) from ImGuiPanels
 * directly to a LiveContextMode enum value.  All other methods are simple
 * comparisons against the stored mode field.
 *
 * Speed multiplier table:
 *   Sunny       → 1.0  (no penalty)
 *   Night       → 1.0  (no speed change, only visual effect)
 *   Incident    → 0.55 (caution zone)
 *   Rain        → 0.65 (wet roads)
 *   HeavyTraffic→ 0.45 (heavy congestion)
 */
#include "LiveContextEngine.h"

void LiveContextEngine::setMode(int selectedMode) {
    // Map the ImGuiPanels combo-box integer index to the corresponding enum.
    switch (selectedMode) {
        case 0:
            mode = LiveContextMode::Sunny;
            break;
        case 1:
            mode = LiveContextMode::Rain;
            break;
        case 2:
            mode = LiveContextMode::Night;
            break;
        case 3:
            mode = LiveContextMode::HeavyTraffic;
            break;
        case 4:
            mode = LiveContextMode::Incident;
            break;
        default:
            mode = LiveContextMode::Sunny; // Fallback to default mode.
            break;
    }
}

LiveContextMode LiveContextEngine::getMode() const {
    return mode;
}

bool LiveContextEngine::isSunnyMode() const {
    return mode == LiveContextMode::Sunny;
}

bool LiveContextEngine::isRainMode() const {
    return mode == LiveContextMode::Rain;
}

bool LiveContextEngine::isNightMode() const {
    return mode == LiveContextMode::Night;
}

bool LiveContextEngine::isHeavyTrafficMode() const {
    return mode == LiveContextMode::HeavyTraffic;
}

bool LiveContextEngine::isIncidentMode() const {
    return mode == LiveContextMode::Incident;
}

float LiveContextEngine::getVehicleSpeedMultiplier() const {
    // Rain slows vehicles: wet roads reduce traction.
    if (mode == LiveContextMode::Rain) {
        return 0.65f;
    }

    // Heavy traffic: vehicles move significantly slower due to congestion.
    if (mode == LiveContextMode::HeavyTraffic) {
        return 0.45f;
    }

    // Incident zone: vehicles slow down as they pass the incident marker.
    if (mode == LiveContextMode::Incident) {
        return 0.55f;
    }

    // Sunny and Night modes do not affect vehicle speed.
    return 1.0f;
}