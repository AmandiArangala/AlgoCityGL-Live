#include "LiveContextEngine.h"

void LiveContextEngine::setMode(int selectedMode) {
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
            mode = LiveContextMode::Sunny;
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
    if (mode == LiveContextMode::Rain) {
        return 0.65f;
    }

    if (mode == LiveContextMode::HeavyTraffic) {
        return 0.45f;
    }

    if (mode == LiveContextMode::Incident) {
        return 0.55f;
    }

    return 1.0f;
}