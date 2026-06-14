#pragma once

class ImGuiPanels {
public:
    void render();

    bool getXRayMode() const;
    int getSelectedLineAlgorithm() const;
    int getSelectedArea() const;
    int getSelectedWeatherMode() const;
    bool getIsometricMode() const;
    bool getIsPlaying() const;

    bool consumeLoadAreaRequest();
    bool consumeResetRequest();

private:
    bool isPlaying = false;
    bool xrayMode = false;
    bool loadAreaRequested = false;
    bool resetRequested = false;

    int selectedArea = 2;
    int selectedWeather = 0;
    int selectedLineAlgorithm = 0;
    int selectedViewMode = 0;

    const char* areas[4] = {
        "University of Moratuwa",
        "Borella Junction",
        "Traffic Demo Location",
        "Random City Location"
    };

    const char* weatherModes[5] = {
        "Sunny",
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