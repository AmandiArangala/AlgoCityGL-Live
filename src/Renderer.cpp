#include <glad/glad.h>

#include "Renderer.h"
#include "LineAlgorithms.h"
#include "CircleAlgorithms.h"
#include "Projection2_5D.h"
#include "FillAlgorithms.h"

#include "imgui.h"

#include <vector>
#include <cstdio>
#include <string>
#include <cmath>

void Renderer::initialize() {
    glClearColor(0.08f, 0.10f, 0.13f, 1.0f);
}

void Renderer::renderBackground() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm) {
    buildDay2PixelScene(selectedLineAlgorithm);
    drawPixelBuffer(xrayMode);
}

void Renderer::renderCityArea(
    const CityArea& area,
    const std::vector<Vehicle>& vehicles,
    const std::vector<RuntimeTrafficLight>& trafficLights,
    const LiveContextEngine& liveContext,
    bool xrayMode,
    int selectedLineAlgorithm,
    bool isometricMode,
    const Camera2D& camera
) {
    drawLiveContextOverlay(liveContext);

    if (isometricMode) {
        drawIsometricRoadFills(area, camera);
        drawBuildingFills2_5D(area, camera);
    } else {
        drawTopDownRoadFills(area, camera);
        drawTopDownBuildingFills(area, camera);
    }

    if (liveContext.isNightMode()) {
        drawNightEffect(area, isometricMode, camera);
    }

    if (liveContext.isIncidentMode()) {
        drawIncidentMarker(isometricMode, camera);
    }

    buildCityPixelScene(area, selectedLineAlgorithm, xrayMode, isometricMode, camera);
    drawPixelBuffer(xrayMode);

    drawEnvironmentDetails(area, isometricMode, camera);
    drawTrees(area, isometricMode, camera);
    drawBuildingWindows(area, isometricMode, camera);
    drawStopLines(trafficLights, isometricMode, camera);

    drawRuntimeTrafficLights(trafficLights, isometricMode, camera);
    drawVehicles(vehicles, isometricMode, camera);
    drawMiniMap(area, vehicles, camera);

    drawRoadLabels(area, isometricMode, camera);
    drawBuildingLabels(area, isometricMode, camera);

    if (liveContext.isRainMode()) {
        drawRainEffect();
    }

    if (xrayMode) {
        drawXRayDashboard(
            area,
            vehicles,
            trafficLights,
            selectedLineAlgorithm,
            isometricMode,
            camera
        );
    }
}

Vec2 Renderer::transformForView(const Vec2& point, bool isometricMode) {
    if (isometricMode) {
        return Projection2_5D::projectPoint(point);
    }

    return point;
}

Vec2 Renderer::applyCamera(const Vec2& point, const Camera2D& camera) {
    return camera.worldToScreen(point);
}

void Renderer::drawTopDownRoadFills(const CityArea& area, const Camera2D& camera) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = applyCamera(road.points[i], camera);
            Vec2 b = applyCamera(road.points[i + 1], camera);

            drawList->AddLine(
                ImVec2(a.x, a.y),
                ImVec2(b.x, b.y),
                IM_COL32(60, 60, 65, 230),
                18.0f * camera.getZoom()
            );

            drawList->AddLine(
                ImVec2(a.x, a.y),
                ImVec2(b.x, b.y),
                IM_COL32(230, 220, 80, 220),
                3.0f * camera.getZoom()
            );
        }
    }
}

void Renderer::drawIsometricRoadFills(const CityArea& area, const Camera2D& camera) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = transformForView(road.points[i], true);
            Vec2 b = transformForView(road.points[i + 1], true);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            drawList->AddLine(
                ImVec2(a.x, a.y),
                ImVec2(b.x, b.y),
                IM_COL32(55, 55, 60, 230),
                18.0f * camera.getZoom()
            );

            drawList->AddLine(
                ImVec2(a.x, a.y),
                ImVec2(b.x, b.y),
                IM_COL32(230, 220, 80, 220),
                3.0f * camera.getZoom()
            );
        }
    }
}

void Renderer::drawTopDownBuildingFills(const CityArea& area, const Camera2D& camera) {
    PixelBuffer fillBuffer;

    Color fillColor(0.05f, 0.45f, 0.60f, 0.45f);

    for (const Building& building : area.buildings) {
        if (building.base.size() >= 3) {
            std::vector<Vec2> screenPolygon;

            for (const Vec2& point : building.base) {
                screenPolygon.push_back(applyCamera(point, camera));
            }

            FillAlgorithms::scanLineFillPolygon(
                fillBuffer,
                screenPolygon,
                fillColor
            );
        }
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Pixel& pixel : fillBuffer.getPixels()) {
        ImU32 color = IM_COL32(
            static_cast<int>(pixel.color.r * 255.0f),
            static_cast<int>(pixel.color.g * 255.0f),
            static_cast<int>(pixel.color.b * 255.0f),
            130
        );

        drawList->AddRectFilled(
            ImVec2(static_cast<float>(pixel.x), static_cast<float>(pixel.y)),
            ImVec2(static_cast<float>(pixel.x + 2), static_cast<float>(pixel.y + 2)),
            color
        );
    }
}

void Renderer::drawBuildingFills2_5D(const CityArea& area, const Camera2D& camera) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Building& building : area.buildings) {
        if (building.base.size() < 3) {
            continue;
        }

        std::vector<ImVec2> base;
        std::vector<ImVec2> top;

        for (const Vec2& point : building.base) {
            Vec2 projected = Projection2_5D::projectPoint(point);
            Vec2 projectedTop = Projection2_5D::shiftUp(projected, building.height);

            projected = applyCamera(projected, camera);
            projectedTop = applyCamera(projectedTop, camera);

            base.push_back(ImVec2(projected.x, projected.y));
            top.push_back(ImVec2(projectedTop.x, projectedTop.y));
        }

        std::vector<ImVec2> shadow;
        for (const ImVec2& p : base) {
            shadow.push_back(ImVec2(
                p.x + 25.0f * camera.getZoom(),
                p.y + 18.0f * camera.getZoom()
            ));
        }

        drawList->AddConvexPolyFilled(
            shadow.data(),
            static_cast<int>(shadow.size()),
            IM_COL32(0, 0, 0, 80)
        );

        for (size_t i = 0; i < base.size(); i++) {
            size_t next = (i + 1) % base.size();

            ImVec2 sideFace[4] = {
                base[i],
                base[next],
                top[next],
                top[i]
            };

            drawList->AddConvexPolyFilled(
                sideFace,
                4,
                IM_COL32(40, 120, 160, 180)
            );
        }

        drawList->AddConvexPolyFilled(
            top.data(),
            static_cast<int>(top.size()),
            IM_COL32(80, 200, 240, 220)
        );
    }
}

void Renderer::buildCityPixelScene(
    const CityArea& area,
    int selectedLineAlgorithm,
    bool xrayMode,
    bool isometricMode,
    const Camera2D& camera
) {
    pixelBuffer.clear();

    Color roadColor(0.75f, 0.75f, 0.75f, 1.0f);
    Color buildingColor(0.2f, 0.8f, 1.0f, 1.0f);
    Color routeColor(1.0f, 0.9f, 0.1f, 1.0f);
    Color crossingColor(1.0f, 1.0f, 1.0f, 1.0f);

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = transformForView(road.points[i], isometricMode);
            Vec2 b = transformForView(road.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            if (selectedLineAlgorithm == 0) {
                LineAlgorithms::drawLineDDA(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    roadColor
                );
            } else {
                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    roadColor
                );
            }
        }
    }

    if (xrayMode) {
        for (const VehicleRoute& route : area.routes) {
            for (size_t i = 0; i + 1 < route.points.size(); i++) {
                Vec2 a = transformForView(route.points[i], isometricMode);
                Vec2 b = transformForView(route.points[i + 1], isometricMode);

                a = applyCamera(a, camera);
                b = applyCamera(b, camera);

                LineAlgorithms::drawLineDDA(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    routeColor
                );
            }
        }
    }

    for (const Building& building : area.buildings) {
        if (building.base.size() < 2) {
            continue;
        }

        if (!isometricMode) {
            for (size_t i = 0; i < building.base.size(); i++) {
                Vec2 a = applyCamera(building.base[i], camera);
                Vec2 b = applyCamera(building.base[(i + 1) % building.base.size()], camera);

                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    buildingColor
                );
            }
        } else {
            std::vector<Vec2> base;
            std::vector<Vec2> top;

            for (const Vec2& point : building.base) {
                Vec2 projected = Projection2_5D::projectPoint(point);
                Vec2 projectedTop = Projection2_5D::shiftUp(projected, building.height);

                projected = applyCamera(projected, camera);
                projectedTop = applyCamera(projectedTop, camera);

                base.push_back(projected);
                top.push_back(projectedTop);
            }

            for (size_t i = 0; i < base.size(); i++) {
                size_t next = (i + 1) % base.size();

                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(base[i].x), static_cast<int>(base[i].y),
                    static_cast<int>(base[next].x), static_cast<int>(base[next].y),
                    buildingColor
                );

                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(top[i].x), static_cast<int>(top[i].y),
                    static_cast<int>(top[next].x), static_cast<int>(top[next].y),
                    buildingColor
                );

                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(base[i].x), static_cast<int>(base[i].y),
                    static_cast<int>(top[i].x), static_cast<int>(top[i].y),
                    buildingColor
                );
            }
        }
    }

    for (const PedestrianCrossing& crossing : area.crossings) {
        for (size_t i = 0; i + 1 < crossing.points.size(); i++) {
            Vec2 a = transformForView(crossing.points[i], isometricMode);
            Vec2 b = transformForView(crossing.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            LineAlgorithms::drawLineBresenham(
                pixelBuffer,
                static_cast<int>(a.x), static_cast<int>(a.y),
                static_cast<int>(b.x), static_cast<int>(b.y),
                crossingColor
            );
        }
    }

    // Traffic lights are drawn dynamically by drawRuntimeTrafficLights().
}

void Renderer::drawRuntimeTrafficLights(
    const std::vector<RuntimeTrafficLight>& trafficLights,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const RuntimeTrafficLight& light : trafficLights) {
        Vec2 pos = transformForView(light.baseLight.position, isometricMode);
        pos = applyCamera(pos, camera);

        float radius = 7.0f * camera.getZoom();
        float spacing = 18.0f * camera.getZoom();

        if (radius < 4.0f) {
            radius = 4.0f;
        }

        ImVec2 redPos(pos.x, pos.y - spacing);
        ImVec2 yellowPos(pos.x, pos.y);
        ImVec2 greenPos(pos.x, pos.y + spacing);

        ImU32 redColor;
        ImU32 yellowColor;
        ImU32 greenColor;

        if (light.state == SignalState::Red) {
            redColor = IM_COL32(255, 40, 40, 255);
            yellowColor = IM_COL32(70, 70, 20, 180);
            greenColor = IM_COL32(20, 70, 20, 180);
        } else if (light.state == SignalState::Yellow) {
            redColor = IM_COL32(70, 20, 20, 180);
            yellowColor = IM_COL32(255, 230, 30, 255);
            greenColor = IM_COL32(20, 70, 20, 180);
        } else {
            redColor = IM_COL32(70, 20, 20, 180);
            yellowColor = IM_COL32(70, 70, 20, 180);
            greenColor = IM_COL32(40, 255, 80, 255);
        }

        drawList->AddRectFilled(
            ImVec2(pos.x - 11.0f * camera.getZoom(), pos.y - spacing - 12.0f * camera.getZoom()),
            ImVec2(pos.x + 11.0f * camera.getZoom(), pos.y + spacing + 12.0f * camera.getZoom()),
            IM_COL32(25, 25, 25, 230),
            4.0f
        );

        drawList->AddCircleFilled(redPos, radius, redColor);
        drawList->AddCircleFilled(yellowPos, radius, yellowColor);
        drawList->AddCircleFilled(greenPos, radius, greenColor);
    }
}

void Renderer::drawVehicles(
    const std::vector<Vehicle>& vehicles,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    auto lerp = [](const ImVec2& a, const ImVec2& b, float t) {
        return ImVec2(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    };

    int vehicleIndex = 0;

    for (const Vehicle& vehicle : vehicles) {
        const std::vector<Vec2>& vertices = vehicle.getTransformedVertices();

        if (vertices.size() < 4) {
            continue;
        }

        std::vector<ImVec2> screenVertices;
        screenVertices.reserve(vertices.size());

        for (const Vec2& vertex : vertices) {
            Vec2 screenPoint = transformForView(vertex, isometricMode);
            screenPoint = applyCamera(screenPoint, camera);

            screenVertices.push_back(ImVec2(screenPoint.x, screenPoint.y));
        }

        if (screenVertices.size() < 4) {
            continue;
        }

        float z = camera.getZoom();

        if (z < 0.35f) {
            z = 0.35f;
        }

        ImVec2 p0 = screenVertices[0];
        ImVec2 p1 = screenVertices[1];
        ImVec2 p2 = screenVertices[2];
        ImVec2 p3 = screenVertices[3];

        ImU32 bodyColor;

        if (vehicle.getIsStopped()) {
            bodyColor = IM_COL32(230, 45, 45, 245);
        } else {
            switch (vehicleIndex % 5) {
                case 0:
                    bodyColor = IM_COL32(255, 120, 40, 245);   // orange
                    break;
                case 1:
                    bodyColor = IM_COL32(60, 150, 255, 245);   // blue
                    break;
                case 2:
                    bodyColor = IM_COL32(240, 240, 245, 245);  // white
                    break;
                case 3:
                    bodyColor = IM_COL32(255, 210, 50, 245);   // yellow
                    break;
                default:
                    bodyColor = IM_COL32(220, 70, 180, 245);   // pink/purple
                    break;
            }
        }

        // Shadow under vehicle
        std::vector<ImVec2> shadowVertices;
        for (const ImVec2& p : screenVertices) {
            shadowVertices.push_back(ImVec2(
                p.x + 5.0f * z,
                p.y + 7.0f * z
            ));
        }

        drawList->AddConvexPolyFilled(
            shadowVertices.data(),
            static_cast<int>(shadowVertices.size()),
            IM_COL32(0, 0, 0, 90)
        );

        // Main car body
        drawList->AddConvexPolyFilled(
            screenVertices.data(),
            static_cast<int>(screenVertices.size()),
            bodyColor
        );

        // Body outline
        drawList->AddPolyline(
            screenVertices.data(),
            static_cast<int>(screenVertices.size()),
            IM_COL32(255, 255, 255, 230),
            ImDrawFlags_Closed,
            2.0f * z
        );

        // Inner roof/window shape
        ImVec2 roof0 = lerp(p0, p3, 0.35f);
        ImVec2 roof1 = lerp(p1, p2, 0.35f);
        ImVec2 roof2 = lerp(p1, p2, 0.68f);
        ImVec2 roof3 = lerp(p0, p3, 0.68f);

        ImVec2 roof[4] = {
            roof0,
            roof1,
            roof2,
            roof3
        };

        drawList->AddConvexPolyFilled(
            roof,
            4,
            IM_COL32(35, 55, 70, 220)
        );

        drawList->AddPolyline(
            roof,
            4,
            IM_COL32(180, 230, 255, 220),
            ImDrawFlags_Closed,
            1.2f * z
        );

        // Front windshield highlight
        ImVec2 wind0 = lerp(p0, p1, 0.25f);
        ImVec2 wind1 = lerp(p0, p1, 0.75f);
        ImVec2 wind2 = lerp(roof0, roof1, 0.75f);
        ImVec2 wind3 = lerp(roof0, roof1, 0.25f);

        ImVec2 windshield[4] = {
            wind0,
            wind1,
            wind2,
            wind3
        };

        drawList->AddConvexPolyFilled(
            windshield,
            4,
            IM_COL32(120, 210, 255, 150)
        );

        // Wheels
        ImVec2 wheelA = lerp(p0, p3, 0.78f);
        ImVec2 wheelB = lerp(p1, p2, 0.78f);
        ImVec2 wheelC = lerp(p0, p3, 0.22f);
        ImVec2 wheelD = lerp(p1, p2, 0.22f);

        float wheelRadius = 3.8f * z;

        drawList->AddCircleFilled(wheelA, wheelRadius, IM_COL32(10, 10, 10, 255));
        drawList->AddCircleFilled(wheelB, wheelRadius, IM_COL32(10, 10, 10, 255));
        drawList->AddCircleFilled(wheelC, wheelRadius, IM_COL32(10, 10, 10, 255));
        drawList->AddCircleFilled(wheelD, wheelRadius, IM_COL32(10, 10, 10, 255));

        drawList->AddCircleFilled(wheelA, wheelRadius * 0.45f, IM_COL32(160, 160, 160, 255));
        drawList->AddCircleFilled(wheelB, wheelRadius * 0.45f, IM_COL32(160, 160, 160, 255));
        drawList->AddCircleFilled(wheelC, wheelRadius * 0.45f, IM_COL32(160, 160, 160, 255));
        drawList->AddCircleFilled(wheelD, wheelRadius * 0.45f, IM_COL32(160, 160, 160, 255));

        // Headlights on front side
        ImVec2 headLight1 = lerp(p0, p1, 0.25f);
        ImVec2 headLight2 = lerp(p0, p1, 0.75f);

        drawList->AddCircleFilled(
            headLight1,
            2.8f * z,
            IM_COL32(255, 245, 150, 255)
        );

        drawList->AddCircleFilled(
            headLight2,
            2.8f * z,
            IM_COL32(255, 245, 150, 255)
        );

        // Red rear lights
        ImVec2 rearLight1 = lerp(p3, p2, 0.25f);
        ImVec2 rearLight2 = lerp(p3, p2, 0.75f);

        drawList->AddCircleFilled(
            rearLight1,
            2.4f * z,
            IM_COL32(255, 40, 40, 230)
        );

        drawList->AddCircleFilled(
            rearLight2,
            2.4f * z,
            IM_COL32(255, 40, 40, 230)
        );

        vehicleIndex++;
    }
}

void Renderer::drawMiniMap(
    const CityArea& area,
    const std::vector<Vehicle>& vehicles,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImVec2 mapSize(260.0f, 180.0f);

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float margin = 20.0f;

    ImVec2 mapPos(
        displaySize.x - mapSize.x - margin,
        margin
    );

    drawList->AddRectFilled(
        mapPos,
        ImVec2(mapPos.x + mapSize.x, mapPos.y + mapSize.y),
        IM_COL32(15, 20, 25, 220),
        8.0f
    );

    drawList->AddRect(
        mapPos,
        ImVec2(mapPos.x + mapSize.x, mapPos.y + mapSize.y),
        IM_COL32(120, 180, 220, 220),
        8.0f,
        0,
        2.0f
    );

    drawList->AddText(
        ImVec2(mapPos.x + 10.0f, mapPos.y + 8.0f),
        IM_COL32(255, 255, 255, 255),
        "Mini Map"
    );

    float minX = 999999.0f;
    float minY = 999999.0f;
    float maxX = -999999.0f;
    float maxY = -999999.0f;

    auto updateBounds = [&](const Vec2& p) {
        if (p.x < minX) minX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.x > maxX) maxX = p.x;
        if (p.y > maxY) maxY = p.y;
    };

    for (const Road& road : area.roads) {
        for (const Vec2& p : road.points) {
            updateBounds(p);
        }
    }

    for (const Building& building : area.buildings) {
        for (const Vec2& p : building.base) {
            updateBounds(p);
        }
    }

    if (minX > maxX || minY > maxY) {
        return;
    }

    float padding = 20.0f;
    float usableW = mapSize.x - 2.0f * padding;
    float usableH = mapSize.y - 2.0f * padding - 20.0f;

    float worldW = maxX - minX;
    float worldH = maxY - minY;

    if (worldW <= 0.0f) worldW = 1.0f;
    if (worldH <= 0.0f) worldH = 1.0f;

    auto toMiniMap = [&](const Vec2& p) {
        float nx = (p.x - minX) / worldW;
        float ny = (p.y - minY) / worldH;

        return ImVec2(
            mapPos.x + padding + nx * usableW,
            mapPos.y + 35.0f + ny * usableH
        );
    };

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            ImVec2 a = toMiniMap(road.points[i]);
            ImVec2 b = toMiniMap(road.points[i + 1]);

            drawList->AddLine(a, b, IM_COL32(180, 180, 180, 255), 2.0f);
        }
    }

    for (const Building& building : area.buildings) {
        for (size_t i = 0; i < building.base.size(); i++) {
            ImVec2 a = toMiniMap(building.base[i]);
            ImVec2 b = toMiniMap(building.base[(i + 1) % building.base.size()]);

            drawList->AddLine(a, b, IM_COL32(80, 200, 240, 255), 1.5f);
        }
    }

    for (const Vehicle& vehicle : vehicles) {
        ImVec2 p = toMiniMap(vehicle.getPosition());

        drawList->AddCircleFilled(
            p,
            4.0f,
            IM_COL32(255, 120, 40, 255)
        );
    }

    char buffer[128];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "Zoom %.2f  Pan(%.0f, %.0f)",
        camera.getZoom(),
        camera.getOffsetX(),
        camera.getOffsetY()
    );

    drawList->AddText(
        ImVec2(mapPos.x + 10.0f, mapPos.y + mapSize.y - 22.0f),
        IM_COL32(220, 220, 220, 255),
        buffer
    );
}

void Renderer::drawLiveContextOverlay(const LiveContextEngine& liveContext) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    if (liveContext.isNightMode()) {
        drawList->AddRectFilled(
            ImVec2(0, 0),
            displaySize,
            IM_COL32(0, 0, 25, 90)
        );
    }

    if (liveContext.isRainMode()) {
        drawList->AddRectFilled(
            ImVec2(0, 0),
            displaySize,
            IM_COL32(20, 30, 45, 70)
        );
    }

    if (liveContext.isHeavyTrafficMode()) {
        drawList->AddText(
            ImVec2(20, 20),
            IM_COL32(255, 180, 60, 255),
            "Live Context: Heavy Traffic - vehicles slowed"
        );
    }

    if (liveContext.isIncidentMode()) {
        drawList->AddText(
            ImVec2(20, 20),
            IM_COL32(255, 80, 80, 255),
            "Live Context: Incident Mode - slow/blocked area active"
        );
    }
}

void Renderer::drawRainEffect() {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    for (int i = 0; i < 90; i++) {
        float x = static_cast<float>((i * 97) % static_cast<int>(displaySize.x));
        float y = static_cast<float>((i * 53) % static_cast<int>(displaySize.y));

        drawList->AddLine(
            ImVec2(x, y),
            ImVec2(x - 8.0f, y + 20.0f),
            IM_COL32(150, 190, 255, 120),
            1.2f
        );
    }
}

void Renderer::drawNightEffect(const CityArea& area, bool isometricMode, const Camera2D& camera) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Building& building : area.buildings) {
        if (building.base.empty()) {
            continue;
        }

        Vec2 center(0.0f, 0.0f);

        for (const Vec2& p : building.base) {
            center.x += p.x;
            center.y += p.y;
        }

        center.x /= static_cast<float>(building.base.size());
        center.y /= static_cast<float>(building.base.size());

        center = transformForView(center, isometricMode);
        center = applyCamera(center, camera);

        drawList->AddCircleFilled(
            ImVec2(center.x, center.y),
            5.0f * camera.getZoom(),
            IM_COL32(255, 230, 120, 180)
        );
    }
}

void Renderer::drawIncidentMarker(bool isometricMode, const Camera2D& camera) {
    Vec2 incidentPoint(430.0f, 410.0f);

    Vec2 p = transformForView(incidentPoint, isometricMode);
    p = applyCamera(p, camera);

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    drawList->AddCircleFilled(
        ImVec2(p.x, p.y),
        22.0f * camera.getZoom(),
        IM_COL32(255, 40, 40, 90)
    );

    drawList->AddTriangleFilled(
        ImVec2(p.x, p.y - 25.0f),
        ImVec2(p.x - 22.0f, p.y + 18.0f),
        ImVec2(p.x + 22.0f, p.y + 18.0f),
        IM_COL32(255, 180, 30, 220)
    );

    drawList->AddText(
        ImVec2(p.x - 18.0f, p.y - 5.0f),
        IM_COL32(0, 0, 0, 255),
        "!"
    );
}

void Renderer::drawXRayDashboard(
    const CityArea& area,
    const std::vector<Vehicle>& vehicles,
    const std::vector<RuntimeTrafficLight>& trafficLights,
    int selectedLineAlgorithm,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImVec2 panelPos(20.0f, 80.0f);
    ImVec2 panelSize(360.0f, 210.0f);

    drawList->AddRectFilled(
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        IM_COL32(10, 15, 20, 230),
        8.0f
    );

    drawList->AddRect(
        panelPos,
        ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
        IM_COL32(120, 200, 255, 220),
        8.0f,
        0,
        2.0f
    );

    const char* lineAlgorithm = selectedLineAlgorithm == 0 ? "DDA Line Algorithm" : "Bresenham Line Algorithm";
    const char* viewMode = isometricMode ? "2.5D Isometric Projection" : "Top-Down 2D View";

    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 12), IM_COL32(255, 255, 255, 255), "Algorithm X-Ray Dashboard");

    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 42), IM_COL32(230, 230, 230, 255), lineAlgorithm);
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 62), IM_COL32(230, 230, 230, 255), "Circle: Midpoint Circle Algorithm");
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 82), IM_COL32(230, 230, 230, 255), "Fill: Scan-line Polygon Fill");
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 102), IM_COL32(230, 230, 230, 255), viewMode);
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 122), IM_COL32(230, 230, 230, 255), "Transform: Translation x Rotation x Scaling");

    char buffer[128];

    std::snprintf(buffer, sizeof(buffer), "Roads: %d  Buildings: %d", static_cast<int>(area.roads.size()), static_cast<int>(area.buildings.size()));
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 152), IM_COL32(180, 220, 255, 255), buffer);

    std::snprintf(buffer, sizeof(buffer), "Vehicles: %d  Signals: %d", static_cast<int>(vehicles.size()), static_cast<int>(trafficLights.size()));
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 172), IM_COL32(180, 220, 255, 255), buffer);

    std::snprintf(buffer, sizeof(buffer), "Camera Zoom: %.2f", camera.getZoom());
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 192), IM_COL32(180, 220, 255, 255), buffer);
}

bool Renderer::shouldShowLabel(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    // Hide automatically generated names.
    if (name.rfind("Building ", 0) == 0) {
        return false;
    }

    if (name.rfind("Road ", 0) == 0) {
        return false;
    }

    // Avoid very long labels cluttering the screen too much.
    if (name.length() > 38) {
        return false;
    }

    return true;
}

Vec2 Renderer::getPolygonCenter(
    const std::vector<Vec2>& points,
    bool isometricMode,
    const Camera2D& camera
) {
    Vec2 center(0.0f, 0.0f);

    if (points.empty()) {
        return center;
    }

    for (const Vec2& point : points) {
        center.x += point.x;
        center.y += point.y;
    }

    center.x /= static_cast<float>(points.size());
    center.y /= static_cast<float>(points.size());

    center = transformForView(center, isometricMode);
    center = applyCamera(center, camera);

    return center;
}

Vec2 Renderer::getRoadLabelPoint(
    const Road& road,
    bool isometricMode,
    const Camera2D& camera
) {
    if (road.points.empty()) {
        return Vec2(0.0f, 0.0f);
    }

    Vec2 point = road.points[road.points.size() / 2];

    point = transformForView(point, isometricMode);
    point = applyCamera(point, camera);

    return point;
}

void Renderer::drawBuildingLabels(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    int shownLabels = 0;
    const int maxLabels = 14;

    for (const Building& building : area.buildings) {
        if (!shouldShowLabel(building.name)) {
            continue;
        }

        Vec2 pos = getPolygonCenter(building.base, isometricMode, camera);

        // Move label slightly above the building.
        pos.y -= (18.0f * camera.getZoom());

        const char* text = building.name.c_str();

        ImVec2 textSize = ImGui::CalcTextSize(text);

        ImVec2 boxMin(
            pos.x - textSize.x * 0.5f - 5.0f,
            pos.y - 4.0f
        );

        ImVec2 boxMax(
            pos.x + textSize.x * 0.5f + 5.0f,
            pos.y + textSize.y + 4.0f
        );

        drawList->AddRectFilled(
            boxMin,
            boxMax,
            IM_COL32(5, 15, 20, 190),
            4.0f
        );

        drawList->AddRect(
            boxMin,
            boxMax,
            IM_COL32(80, 210, 255, 180),
            4.0f,
            0,
            1.0f
        );

        drawList->AddText(
            ImVec2(pos.x - textSize.x * 0.5f, pos.y),
            IM_COL32(230, 250, 255, 255),
            text
        );

        shownLabels++;

        if (shownLabels >= maxLabels) {
            break;
        }
    }
}

void Renderer::drawRoadLabels(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    int shownLabels = 0;
    const int maxLabels = 8;

    for (const Road& road : area.roads) {
        if (!shouldShowLabel(road.name)) {
            continue;
        }

        Vec2 pos = getRoadLabelPoint(road, isometricMode, camera);

        const char* text = road.name.c_str();

        ImVec2 textSize = ImGui::CalcTextSize(text);

        ImVec2 boxMin(
            pos.x - textSize.x * 0.5f - 5.0f,
            pos.y - 20.0f
        );

        ImVec2 boxMax(
            pos.x + textSize.x * 0.5f + 5.0f,
            pos.y - 20.0f + textSize.y + 8.0f
        );

        drawList->AddRectFilled(
            boxMin,
            boxMax,
            IM_COL32(35, 35, 20, 180),
            4.0f
        );

        drawList->AddRect(
            boxMin,
            boxMax,
            IM_COL32(255, 230, 100, 180),
            4.0f,
            0,
            1.0f
        );

        drawList->AddText(
            ImVec2(pos.x - textSize.x * 0.5f, pos.y - 16.0f),
            IM_COL32(255, 240, 150, 255),
            text
        );

        shownLabels++;

        if (shownLabels >= maxLabels) {
            break;
        }
    }
}

void Renderer::drawEnvironmentDetails(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Light sidewalk/road shoulder lines beside each road.
    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = transformForView(road.points[i], isometricMode);
            Vec2 b = transformForView(road.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float length = std::sqrt(dx * dx + dy * dy);

            if (length <= 0.01f) {
                continue;
            }

            float nx = -dy / length;
            float ny = dx / length;

            float offset = 13.0f * camera.getZoom();

            ImVec2 leftA(a.x + nx * offset, a.y + ny * offset);
            ImVec2 leftB(b.x + nx * offset, b.y + ny * offset);

            ImVec2 rightA(a.x - nx * offset, a.y - ny * offset);
            ImVec2 rightB(b.x - nx * offset, b.y - ny * offset);

            drawList->AddLine(
                leftA,
                leftB,
                IM_COL32(130, 130, 135, 120),
                2.0f * camera.getZoom()
            );

            drawList->AddLine(
                rightA,
                rightB,
                IM_COL32(130, 130, 135, 120),
                2.0f * camera.getZoom()
            );
        }
    }

    // Small junction circles to make intersections clearer.
    for (const TrafficLight& light : area.trafficLights) {
        Vec2 p = transformForView(light.position, isometricMode);
        p = applyCamera(p, camera);

        drawList->AddCircleFilled(
            ImVec2(p.x, p.y),
            22.0f * camera.getZoom(),
            IM_COL32(60, 60, 65, 130)
        );

        drawList->AddCircle(
            ImVec2(p.x, p.y),
            22.0f * camera.getZoom(),
            IM_COL32(210, 210, 210, 130),
            24,
            1.5f * camera.getZoom()
        );
    }
}

void Renderer::drawTrees(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    int treeCount = 0;
    const int maxTrees = 90;

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 p1 = road.points[i];
            Vec2 p2 = road.points[i + 1];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float length = std::sqrt(dx * dx + dy * dy);

            if (length < 80.0f) {
                continue;
            }

            float nx = -dy / length;
            float ny = dx / length;

            int samples = static_cast<int>(length / 130.0f);

            for (int s = 1; s <= samples; s++) {
                float t = static_cast<float>(s) / static_cast<float>(samples + 1);

                Vec2 basePoint(
                    p1.x + dx * t,
                    p1.y + dy * t
                );

                // Alternate sides of the road.
                float side = (s % 2 == 0) ? 1.0f : -1.0f;

                Vec2 treeWorld(
                    basePoint.x + nx * side * 36.0f,
                    basePoint.y + ny * side * 36.0f
                );

                Vec2 screen = transformForView(treeWorld, isometricMode);
                screen = applyCamera(screen, camera);

                float z = camera.getZoom();

                // Tree shadow
                drawList->AddCircleFilled(
                    ImVec2(screen.x + 4.0f * z, screen.y + 6.0f * z),
                    8.0f * z,
                    IM_COL32(0, 0, 0, 70)
                );

                // Trunk
                drawList->AddRectFilled(
                    ImVec2(screen.x - 2.0f * z, screen.y + 4.0f * z),
                    ImVec2(screen.x + 2.0f * z, screen.y + 13.0f * z),
                    IM_COL32(100, 65, 30, 220)
                );

                // Leaves
                drawList->AddCircleFilled(
                    ImVec2(screen.x, screen.y),
                    9.0f * z,
                    IM_COL32(35, 150, 75, 230)
                );

                drawList->AddCircleFilled(
                    ImVec2(screen.x - 6.0f * z, screen.y + 3.0f * z),
                    7.0f * z,
                    IM_COL32(25, 120, 60, 220)
                );

                drawList->AddCircleFilled(
                    ImVec2(screen.x + 6.0f * z, screen.y + 3.0f * z),
                    7.0f * z,
                    IM_COL32(45, 170, 85, 220)
                );

                treeCount++;

                if (treeCount >= maxTrees) {
                    return;
                }
            }
        }
    }
}

void Renderer::drawBuildingWindows(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    int buildingIndex = 0;

    for (const Building& building : area.buildings) {
        if (building.base.empty()) {
            continue;
        }

        buildingIndex++;

        Vec2 center = getPolygonCenter(
            building.base,
            isometricMode,
            camera
        );

        float z = camera.getZoom();

        // Skip if zoomed too far out.
        if (z < 0.45f) {
            continue;
        }

        int rows = 2 + (buildingIndex % 3);
        int cols = 3 + (buildingIndex % 4);

        float startX = center.x - cols * 5.0f * z;
        float startY = center.y - rows * 6.0f * z;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if ((r + c + buildingIndex) % 3 == 0) {
                    continue;
                }

                float x = startX + c * 10.0f * z;
                float y = startY + r * 9.0f * z;

                drawList->AddRectFilled(
                    ImVec2(x, y),
                    ImVec2(x + 4.0f * z, y + 5.0f * z),
                    IM_COL32(220, 245, 255, 150)
                );
            }
        }
    }
}

void Renderer::drawStopLines(
    const std::vector<RuntimeTrafficLight>& trafficLights,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const RuntimeTrafficLight& light : trafficLights) {
        Vec2 pos = transformForView(light.baseLight.position, isometricMode);
        pos = applyCamera(pos, camera);

        float z = camera.getZoom();

        drawList->AddLine(
            ImVec2(pos.x - 28.0f * z, pos.y + 18.0f * z),
            ImVec2(pos.x + 28.0f * z, pos.y - 10.0f * z),
            IM_COL32(255, 255, 255, 230),
            4.0f * z
        );

        drawList->AddLine(
            ImVec2(pos.x - 30.0f * z, pos.y + 22.0f * z),
            ImVec2(pos.x + 30.0f * z, pos.y - 6.0f * z),
            IM_COL32(30, 30, 30, 130),
            1.0f * z
        );
    }
}

void Renderer::buildDay2PixelScene(int selectedLineAlgorithm) {
    pixelBuffer.clear();

    Color roadColor(0.75f, 0.75f, 0.75f, 1.0f);
    Color laneColor(1.0f, 1.0f, 0.2f, 1.0f);
    Color buildingColor(0.2f, 0.8f, 1.0f, 1.0f);
    Color wheelColor(0.05f, 0.05f, 0.05f, 1.0f);
    Color lightColor(1.0f, 0.1f, 0.1f, 1.0f);

    if (selectedLineAlgorithm == 0) {
        LineAlgorithms::drawLineDDA(pixelBuffer, 120, 500, 850, 250, roadColor);
    } else {
        LineAlgorithms::drawLineBresenham(pixelBuffer, 120, 500, 850, 250, roadColor);
    }

    LineAlgorithms::drawLineBresenham(pixelBuffer, 120, 560, 850, 310, roadColor);
    LineAlgorithms::drawLineDDA(pixelBuffer, 200, 520, 750, 330, laneColor);

    LineAlgorithms::drawLineBresenham(pixelBuffer, 600, 130, 760, 130, buildingColor);
    LineAlgorithms::drawLineBresenham(pixelBuffer, 760, 130, 760, 230, buildingColor);
    LineAlgorithms::drawLineBresenham(pixelBuffer, 760, 230, 600, 230, buildingColor);
    LineAlgorithms::drawLineBresenham(pixelBuffer, 600, 230, 600, 130, buildingColor);

    CircleAlgorithms::drawCircleMidpoint(pixelBuffer, 350, 450, 25, wheelColor);
    CircleAlgorithms::drawCircleMidpoint(pixelBuffer, 450, 420, 25, wheelColor);

    CircleAlgorithms::drawCircleMidpoint(pixelBuffer, 900, 180, 18, lightColor);
    CircleAlgorithms::drawCircleMidpoint(pixelBuffer, 900, 230, 18, Color(1.0f, 1.0f, 0.1f, 1.0f));
    CircleAlgorithms::drawCircleMidpoint(pixelBuffer, 900, 280, 18, Color(0.1f, 1.0f, 0.1f, 1.0f));
}

void Renderer::drawPixelBuffer(bool xrayMode) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    float pixelSize = xrayMode ? 5.0f : 3.0f;

    for (const Pixel& pixel : pixelBuffer.getPixels()) {
        ImU32 color = IM_COL32(
            static_cast<int>(pixel.color.r * 255.0f),
            static_cast<int>(pixel.color.g * 255.0f),
            static_cast<int>(pixel.color.b * 255.0f),
            static_cast<int>(pixel.color.a * 255.0f)
        );

        ImVec2 p1(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
        ImVec2 p2(
            static_cast<float>(pixel.x) + pixelSize,
            static_cast<float>(pixel.y) + pixelSize
        );

        drawList->AddRectFilled(p1, p2, color);

        if (xrayMode) {
            drawList->AddRect(p1, p2, IM_COL32(255, 255, 255, 120));
        }
    }

    if (xrayMode) {
        drawList->AddText(
            ImVec2(80, 650),
            IM_COL32(255, 255, 255, 255),
            "X-Ray Mode: Manual raster pixels + routes + viewing transform visible."
        );
    }
}