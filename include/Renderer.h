#pragma once

#include <vector>

#include "PixelBuffer.h"
#include "CityArea.h"
#include "Vehicle.h"
#include "SignalController.h"

class Renderer {
public:
    void initialize();
    void renderBackground();

    void renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm);

    void renderCityArea(
        const CityArea& area,
        const std::vector<Vehicle>& vehicles,
        const std::vector<RuntimeTrafficLight>& trafficLights,
        bool xrayMode,
        int selectedLineAlgorithm,
        bool isometricMode
    );

private:
    PixelBuffer pixelBuffer;

    void buildDay2PixelScene(int selectedLineAlgorithm);

    void buildCityPixelScene(
        const CityArea& area,
        int selectedLineAlgorithm,
        bool xrayMode,
        bool isometricMode
    );

    Vec2 transformForView(const Vec2& point, bool isometricMode);
    void drawBuildingFills2_5D(const CityArea& area);
    void drawTopDownBuildingFills(const CityArea& area);
    void drawTopDownRoadFills(const CityArea& area);
    void drawPixelBuffer(bool xrayMode);

    void drawVehicles(const std::vector<Vehicle>& vehicles, bool isometricMode);

    void drawRuntimeTrafficLights(
        const std::vector<RuntimeTrafficLight>& trafficLights,
        bool isometricMode
    );

};