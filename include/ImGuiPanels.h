#pragma once

#include <string>

class ImGuiPanels {
public:
    void render();

private:
    bool isPlaying = false;
    bool xrayMode = false;

    int selectedArea = 0;
    int selectedWeather = 0;

    const char* areas[3] = {
        "University of Moratuwa",
        "Pettah / Colombo Fort",
        "Borella Junction"
    };

    const char* weatherModes[5] = {
        "Normal",
        "Rain",
        "Night",
        "Heavy Traffic",
        "Incident"
    };
};