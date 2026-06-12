#pragma once

#include <vector>
#include <string>

#include "PixelBuffer.h"
#include "CityArea.h"
#include "Vehicle.h"
#include "SignalController.h"
#include "Camera2D.h"
#include "LiveContextEngine.h"

struct ImDrawList;

class Renderer {
public:
    void initialize();
    void renderBackground();

    void renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm);

    void renderCityArea(
        const CityArea& area,
        const std::vector<Vehicle>& vehicles,
        const std::vector<RuntimeTrafficLight>& trafficLights,
        const LiveContextEngine& liveContextEngine,
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

    void drawBuildingFills2_5D(const CityArea& area, const LiveContextEngine& liveContext, const Camera2D& camera);
    void drawTopDownBuildingFills(const CityArea& area, const LiveContextEngine& liveContext, const Camera2D& camera);
    void drawRoads(const CityArea& area, bool isometricMode, const Camera2D& camera);

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

    void drawLiveContextOverlay(const LiveContextEngine& liveContext);
    void drawRainEffect();
    void drawNightEffect(const CityArea& area, bool isometricMode, const Camera2D& camera);
    void drawIncidentMarker(bool isometricMode, const Camera2D& camera);
    void drawXRayDashboard(
        const CityArea& area,
        const std::vector<Vehicle>& vehicles,
        const std::vector<RuntimeTrafficLight>& trafficLights,
        int selectedLineAlgorithm,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawBuildingLabels(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawRoadLabels(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );

    bool shouldShowLabel(const std::string& name);

    Vec2 getPolygonCenter(
        const std::vector<Vec2>& points,
        bool isometricMode,
        const Camera2D& camera
    );

    Vec2 getRoadLabelPoint(
        const Road& road,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawGround(
        const CityArea& area,
        const LiveContextEngine& liveContext,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawRoadMarkings(
        ImDrawList* drawList,
        const Vec2& a,
        const Vec2& b,
        int lanes,
        float roadWidth,
        float z
    );

    void drawEnvironmentDetails(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawTrees(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawBuildingWindows(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawStopLines(
        const std::vector<RuntimeTrafficLight>& trafficLights,
        bool isometricMode,
        const Camera2D& camera
    );

    void drawPedestrianCrossings(
        const CityArea& area,
        bool isometricMode,
        const Camera2D& camera
    );
};