#include <glad/glad.h>

#include "Renderer.h"
#include "LineAlgorithms.h"
#include "CircleAlgorithms.h"
#include "Projection2_5D.h"
#include "FillAlgorithms.h"

#include "imgui.h"

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
    bool isometricMode
) {
    if (isometricMode) {
        drawBuildingFills2_5D(area);
    } else {
        drawTopDownRoadFills(area);
        drawTopDownBuildingFills(area);
    }

    buildCityPixelScene(area, selectedLineAlgorithm, xrayMode, isometricMode);
    drawPixelBuffer(xrayMode);

    drawRuntimeTrafficLights(trafficLights, isometricMode);
    drawVehicles(vehicles, isometricMode);
}

Vec2 Renderer::transformForView(const Vec2& point, bool isometricMode) {
    if (isometricMode) {
        return Projection2_5D::projectPoint(point);
    }

    return point;
}

void Renderer::drawBuildingFills2_5D(const CityArea& area) {
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

            base.push_back(ImVec2(projected.x, projected.y));
            top.push_back(ImVec2(projectedTop.x, projectedTop.y));
        }

        // Shadow: simple shifted polygon
        std::vector<ImVec2> shadow;
        for (const ImVec2& p : base) {
            shadow.push_back(ImVec2(p.x + 25.0f, p.y + 18.0f));
        }

        drawList->AddConvexPolyFilled(
            shadow.data(),
            static_cast<int>(shadow.size()),
            IM_COL32(0, 0, 0, 80)
        );

        // Building side faces
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

        // Building top face
        drawList->AddConvexPolyFilled(
            top.data(),
            static_cast<int>(top.size()),
            IM_COL32(80, 200, 240, 220)
        );
    }
}

void Renderer::drawTopDownBuildingFills(const CityArea& area) {
    PixelBuffer fillBuffer;

    Color fillColor(0.05f, 0.45f, 0.60f, 0.45f);

    for (const Building& building : area.buildings) {
        if (building.base.size() >= 3) {
            FillAlgorithms::scanLineFillPolygon(
                fillBuffer,
                building.base,
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

void Renderer::drawTopDownRoadFills(const CityArea& area) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = road.points[i];
            Vec2 b = road.points[i + 1];

            drawList->AddLine(
                ImVec2(a.x, a.y),
                ImVec2(b.x, b.y),
                IM_COL32(60, 60, 65, 220),
                18.0f
            );
        }
    }
}

void Renderer::buildCityPixelScene(
    const CityArea& area,
    int selectedLineAlgorithm,
    bool xrayMode,
    bool isometricMode
) {
    pixelBuffer.clear();

    Color roadColor(0.75f, 0.75f, 0.75f, 1.0f);
    Color buildingColor(0.2f, 0.8f, 1.0f, 1.0f);
    Color routeColor(1.0f, 0.9f, 0.1f, 1.0f);
    Color crossingColor(1.0f, 1.0f, 1.0f, 1.0f);
    Color signalRed(1.0f, 0.1f, 0.1f, 1.0f);

    // Draw roads
    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = transformForView(road.points[i], isometricMode);
            Vec2 b = transformForView(road.points[i + 1], isometricMode);

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

    // Draw vehicle routes only in X-Ray Mode
    if (xrayMode) {
        for (const VehicleRoute& route : area.routes) {
            for (size_t i = 0; i + 1 < route.points.size(); i++) {
                Vec2 a = transformForView(route.points[i], isometricMode);
                Vec2 b = transformForView(route.points[i + 1], isometricMode);

                LineAlgorithms::drawLineDDA(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    routeColor
                );
            }
        }
    }

    // Draw building outlines
    for (const Building& building : area.buildings) {
        if (building.base.size() < 2) {
            continue;
        }

        if (!isometricMode) {
            // Top-down building outline
            for (size_t i = 0; i < building.base.size(); i++) {
                Vec2 a = building.base[i];
                Vec2 b = building.base[(i + 1) % building.base.size()];

                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(a.x), static_cast<int>(a.y),
                    static_cast<int>(b.x), static_cast<int>(b.y),
                    buildingColor
                );
            }
        } else {
            // 2.5D building wireframe outline
            std::vector<Vec2> base;
            std::vector<Vec2> top;

            for (const Vec2& point : building.base) {
                Vec2 projected = Projection2_5D::projectPoint(point);
                Vec2 projectedTop = Projection2_5D::shiftUp(projected, building.height);

                base.push_back(projected);
                top.push_back(projectedTop);
            }

            for (size_t i = 0; i < base.size(); i++) {
                size_t next = (i + 1) % base.size();

                // base outline
                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(base[i].x), static_cast<int>(base[i].y),
                    static_cast<int>(base[next].x), static_cast<int>(base[next].y),
                    buildingColor
                );

                // top outline
                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(top[i].x), static_cast<int>(top[i].y),
                    static_cast<int>(top[next].x), static_cast<int>(top[next].y),
                    buildingColor
                );

                // vertical edge
                LineAlgorithms::drawLineBresenham(
                    pixelBuffer,
                    static_cast<int>(base[i].x), static_cast<int>(base[i].y),
                    static_cast<int>(top[i].x), static_cast<int>(top[i].y),
                    buildingColor
                );
            }
        }
    }

    // Draw pedestrian crossings
    for (const PedestrianCrossing& crossing : area.crossings) {
        for (size_t i = 0; i + 1 < crossing.points.size(); i++) {
            Vec2 a = transformForView(crossing.points[i], isometricMode);
            Vec2 b = transformForView(crossing.points[i + 1], isometricMode);

            LineAlgorithms::drawLineBresenham(
                pixelBuffer,
                static_cast<int>(a.x), static_cast<int>(a.y),
                static_cast<int>(b.x), static_cast<int>(b.y),
                crossingColor
            );
        }
    }

    // Draw traffic lights
    /*
    for (const TrafficLight& light : area.trafficLights) {
        Vec2 pos = transformForView(light.position, isometricMode);

        CircleAlgorithms::drawCircleMidpoint(
            pixelBuffer,
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            14,
            signalRed
        );
    }*/
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

void Renderer::drawRuntimeTrafficLights(
    const std::vector<RuntimeTrafficLight>& trafficLights,
    bool isometricMode
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const RuntimeTrafficLight& light : trafficLights) {
        Vec2 pos = transformForView(light.baseLight.position, isometricMode);

        float radius = 7.0f;
        float spacing = 18.0f;

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

        // Traffic light pole/body
        drawList->AddRectFilled(
            ImVec2(pos.x - 11.0f, pos.y - spacing - 12.0f),
            ImVec2(pos.x + 11.0f, pos.y + spacing + 12.0f),
            IM_COL32(25, 25, 25, 230),
            4.0f
        );

        drawList->AddRect(
            ImVec2(pos.x - 11.0f, pos.y - spacing - 12.0f),
            ImVec2(pos.x + 11.0f, pos.y + spacing + 12.0f),
            IM_COL32(220, 220, 220, 180),
            4.0f,
            0,
            1.5f
        );

        // Three bulbs
        drawList->AddCircleFilled(redPos, radius, redColor);
        drawList->AddCircleFilled(yellowPos, radius, yellowColor);
        drawList->AddCircleFilled(greenPos, radius, greenColor);

        drawList->AddCircle(redPos, radius, IM_COL32(255, 255, 255, 120), 20, 1.0f);
        drawList->AddCircle(yellowPos, radius, IM_COL32(255, 255, 255, 120), 20, 1.0f);
        drawList->AddCircle(greenPos, radius, IM_COL32(255, 255, 255, 120), 20, 1.0f);

        // Optional small pole
        drawList->AddLine(
            ImVec2(pos.x, pos.y + spacing + 12.0f),
            ImVec2(pos.x, pos.y + spacing + 35.0f),
            IM_COL32(180, 180, 180, 180),
            2.0f
        );
    }
}

void Renderer::drawVehicles(const std::vector<Vehicle>& vehicles, bool isometricMode) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const Vehicle& vehicle : vehicles) {
        const std::vector<Vec2>& vertices = vehicle.getTransformedVertices();

        if (vertices.size() < 3) {
            continue;
        }

        std::vector<ImVec2> screenVertices;

        for (const Vec2& vertex : vertices) {
            Vec2 screenPoint = transformForView(vertex, isometricMode);
            screenVertices.push_back(ImVec2(screenPoint.x, screenPoint.y));
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
            "X-Ray Mode: Layout is drawn using manual line/circle algorithms."
        );
    }
}