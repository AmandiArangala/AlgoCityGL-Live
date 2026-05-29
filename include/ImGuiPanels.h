#pragma once

class ImGuiPanels {
public:
    void render();

    bool getXRayMode() const;
    int getSelectedLineAlgorithm() const;
    int getSelectedArea() const;
    bool getIsometricMode() const;

    bool consumeLoadAreaRequest();

private:
    bool isPlaying = false;
    bool xrayMode = false;
    bool loadAreaRequested = false;

    int selectedArea = 0;
    int selectedWeather = 0;
    int selectedLineAlgorithm = 0;
    int selectedViewMode = 0;

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

    const char* lineAlgorithms[2] = {
        "DDA",
        "Bresenham"
    };

    const char* viewModes[2] = {
        "Top-Down 2D",
        "2.5D Isometric"
    };
};