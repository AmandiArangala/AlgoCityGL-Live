#pragma once

enum class LiveContextMode {
    Sunny,
    Rain,
    Night,
    HeavyTraffic,
    Incident
};

class LiveContextEngine {
public:
    void setMode(int selectedMode);
    LiveContextMode getMode() const;

    bool isSunnyMode() const;
    bool isRainMode() const;
    bool isNightMode() const;
    bool isHeavyTrafficMode() const;
    bool isIncidentMode() const;

    float getVehicleSpeedMultiplier() const;

private:
    LiveContextMode mode = LiveContextMode::Sunny;
};