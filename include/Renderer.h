#pragma once

#include <vector>

#include "PixelBuffer.h"
#include "CityArea.h"
#include "Vehicle.h"
#include "SignalController.h"
#include "Camera2D.h"

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
        bool isometricMode,
        const Camera2D& camera
    );

private:
    PixelBuffer pixelBuffer;

    void buildDay2PixelScene(int selectedLineAlgorithm);

    void buildCityPixelScene(
        const CityArea& area,
        int selectedLineAlgorithm,
        bool xrayMode,
        bool isometricMode,
        const Camera2D& camera
    );

    Vec2 transformForView(const Vec2& point, bool isometricMode);
    Vec2 applyCamera(const Vec2& point, const Camera2D& camera);

    void drawBuildingFills2_5D(const CityArea& area, const Camera2D& camera);
    void drawTopDownBuildingFills(const CityArea& area, const Camera2D& camera);
    void drawTopDownRoadFills(const CityArea& area, const Camera2D& camera);
    void drawIsometricRoadFills(const CityArea& area, const Camera2D& camera);

    void drawRuntimeTrafficLights(
        const std::vector<RuntimeTrafficLight>& trafficLights,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawVehicles(
        const std::vector<Vehicle>& vehicles,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawMiniMap(
        const CityArea& area,
        const std::vector<Vehicle>& vehicles,
        const Camera2D& camera
    );

    void drawPixelBuffer(bool xrayMode);
};