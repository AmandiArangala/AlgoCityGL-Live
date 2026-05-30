#include <glad/glad.h>

#include "Renderer.h"
#include "LineAlgorithms.h"
#include "CircleAlgorithms.h"
#include "Projection2_5D.h"
#include "FillAlgorithms.h"

#include "imgui.h"

#include <vector>
#include <cstdio>

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
    bool xrayMode,
    int selectedLineAlgorithm,
    bool isometricMode,
    const Camera2D& camera
) {
    if (isometricMode) {
        drawIsometricRoadFills(area, camera);
        drawBuildingFills2_5D(area, camera);
    } else {
        drawTopDownRoadFills(area, camera);
        drawTopDownBuildingFills(area, camera);
    }

    buildCityPixelScene(area, selectedLineAlgorithm, xrayMode, isometricMode, camera);
    drawPixelBuffer(xrayMode);

    drawRuntimeTrafficLights(trafficLights, isometricMode, camera);
    drawVehicles(vehicles, isometricMode, camera);
    drawMiniMap(area, vehicles, camera);
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

    for (const Vehicle& vehicle : vehicles) {
        const std::vector<Vec2>& vertices = vehicle.getTransformedVertices();

        if (vertices.size() < 3) {
            continue;
        }

        std::vector<ImVec2> screenVertices;
        screenVertices.reserve(vertices.size());

        for (const Vec2& vertex : vertices) {
            Vec2 screenPoint = transformForView(vertex, isometricMode);
            screenPoint = applyCamera(screenPoint, camera);

            screenVertices.push_back(ImVec2(screenPoint.x, screenPoint.y));
        }

        if (screenVertices.size() < 3) {
            continue;
        }

        ImU32 vehicleColor = vehicle.getIsStopped()
            ? IM_COL32(255, 40, 40, 240)
            : IM_COL32(255, 120, 40, 240);

        drawList->AddConvexPolyFilled(
            screenVertices.data(),
            static_cast<int>(screenVertices.size()),
            vehicleColor
        );

        drawList->AddPolyline(
            screenVertices.data(),
            static_cast<int>(screenVertices.size()),
            IM_COL32(255, 255, 255, 255),
            ImDrawFlags_Closed,
            2.0f
        );
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