#include <glad/glad.h>

#include "Renderer.h"
#include "LineAlgorithms.h"
#include "CircleAlgorithms.h"
#include "Projection2_5D.h"
#include "FillAlgorithms.h"
#include "ClippingAlgorithms.h"

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
    // 1. Draw background ground terrain and grid
    drawGround(liveContext);

    // 2. Draw roads and buildings
    drawRoads(area, isometricMode, camera);
    if (isometricMode) {
        drawBuildingFills2_5D(area, liveContext, camera);
    } else {
        drawTopDownBuildingFills(area, liveContext, camera);
    }

    // 3. Draw weather/night tint overlay (so it dims the terrain and structures)
    drawLiveContextOverlay(liveContext);

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
    drawPedestrianCrossings(area, isometricMode, camera);

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

void Renderer::drawRoads(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float z = camera.getZoom();

    struct StagedRoad {
        std::vector<ImVec2> points;
        int lanes;
        float roadWidth;
        float sidewalkWidth;
        float curbWidth;
    };

    std::vector<StagedRoad> stagedRoads;
    stagedRoads.reserve(area.roads.size());

    for (const Road& road : area.roads) {
        if (road.points.size() < 2) {
            continue;
        }

        StagedRoad staged;
        staged.lanes = road.lanes;
        staged.roadWidth = (road.lanes == 1 ? 18.0f : (road.lanes == 2 ? 30.0f : 44.0f)) * z;
        staged.sidewalkWidth = staged.roadWidth + 6.0f * z;
        staged.curbWidth = staged.roadWidth + 2.0f * z;

        staged.points.reserve(road.points.size());
        for (const Vec2& p : road.points) {
            Vec2 proj = transformForView(p, isometricMode);
            proj = applyCamera(proj, camera);
            staged.points.push_back(ImVec2(proj.x, proj.y));
        }
        stagedRoads.push_back(staged);
    }

    // Pass 1: Draw Sidewalks (Concrete Base)
    ImU32 sidewalkColor = IM_COL32(145, 145, 150, 255);
    for (const auto& road : stagedRoads) {
        float r = road.sidewalkWidth * 0.5f;
        for (const auto& p : road.points) {
            drawList->AddCircleFilled(p, r, sidewalkColor);
        }
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            drawList->AddLine(road.points[i], road.points[i + 1], sidewalkColor, road.sidewalkWidth);
        }
    }

    // Pass 2: Draw Curbs (White Highlights)
    ImU32 curbColor = IM_COL32(230, 230, 235, 255);
    for (const auto& road : stagedRoads) {
        float r = road.curbWidth * 0.5f;
        for (const auto& p : road.points) {
            drawList->AddCircleFilled(p, r, curbColor);
        }
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            drawList->AddLine(road.points[i], road.points[i + 1], curbColor, road.curbWidth);
        }
    }

    // Pass 3: Draw Asphalt (Dark Gray Road Bed)
    ImU32 asphaltColor = IM_COL32(40, 40, 44, 255);
    for (const auto& road : stagedRoads) {
        float r = road.roadWidth * 0.5f;
        for (const auto& p : road.points) {
            drawList->AddCircleFilled(p, r, asphaltColor);
        }
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            drawList->AddLine(road.points[i], road.points[i + 1], asphaltColor, road.roadWidth);
        }
    }

    // Pass 4: Draw Lane Markings
    for (const auto& road : stagedRoads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            drawRoadMarkings(
                drawList,
                Vec2(road.points[i].x, road.points[i].y),
                Vec2(road.points[i + 1].x, road.points[i + 1].y),
                road.lanes,
                road.roadWidth,
                z
            );
        }
    }
}

void Renderer::drawRoadMarkings(
    ImDrawList* drawList,
    const Vec2& a,
    const Vec2& b,
    int lanes,
    float roadWidth,
    float z
) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.01f) return;

    float nx = -dy / len;
    float ny = dx / len;

    if (lanes == 1) {
        // Single yellow line for narrow single lanes
        drawList->AddLine(
            ImVec2(a.x, a.y),
            ImVec2(b.x, b.y),
            IM_COL32(235, 195, 50, 200),
            1.5f * z
        );
    } else if (lanes == 2) {
        // Double yellow line for standard two lanes
        float dyell = 2.0f * z;
        drawList->AddLine(
            ImVec2(a.x + nx * dyell, a.y + ny * dyell),
            ImVec2(b.x + nx * dyell, b.y + ny * dyell),
            IM_COL32(235, 195, 50, 210),
            1.2f * z
        );
        drawList->AddLine(
            ImVec2(a.x - nx * dyell, a.y - ny * dyell),
            ImVec2(b.x - nx * dyell, b.y - ny * dyell),
            IM_COL32(235, 195, 50, 210),
            1.2f * z
        );
    } else {
        // 3+ lanes: Double yellow center divider + dashed white lane separators on sides
        float dyell = 2.5f * z;
        drawList->AddLine(
            ImVec2(a.x + nx * dyell, a.y + ny * dyell),
            ImVec2(b.x + nx * dyell, b.y + ny * dyell),
            IM_COL32(235, 195, 50, 210),
            1.2f * z
        );
        drawList->AddLine(
            ImVec2(a.x - nx * dyell, a.y - ny * dyell),
            ImVec2(b.x - nx * dyell, b.y - ny * dyell),
            IM_COL32(235, 195, 50, 210),
            1.2f * z
        );

        // Dashed lines
        float dashLen = 14.0f * z;
        float gapLen = 10.0f * z;
        float totalStep = dashLen + gapLen;
        float offsetDashed = roadWidth * 0.25f;

        auto drawDashed = [&](float sideOffset) {
            float distTravelled = 0.0f;
            while (distTravelled < len) {
                float tStart = distTravelled / len;
                float tEnd = std::min(distTravelled + dashLen, len) / len;

                ImVec2 pStart(
                    a.x + dx * tStart + nx * sideOffset,
                    a.y + dy * tStart + ny * sideOffset
                );
                ImVec2 pEnd(
                    a.x + dx * tEnd + nx * sideOffset,
                    a.y + dy * tEnd + ny * sideOffset
                );

                drawList->AddLine(pStart, pEnd, IM_COL32(255, 255, 255, 160), 1.2f * z);
                distTravelled += totalStep;
            }
        };

        drawDashed(offsetDashed);
        drawDashed(-offsetDashed);
    }
}

void Renderer::drawTopDownBuildingFills(
    const CityArea& area,
    const LiveContextEngine& liveContext,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float z = camera.getZoom();

    auto lerp = [](const ImVec2& a, const ImVec2& b, float t) {
        return ImVec2(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    };

    int buildingIndex = 0;

    for (const Building& building : area.buildings) {
        if (building.base.size() < 3) {
            continue;
        }

        buildingIndex++;

        // Compute centroid
        Vec2 worldCenter(0.0f, 0.0f);
        for (const Vec2& pt : building.base) {
            worldCenter.x += pt.x;
            worldCenter.y += pt.y;
        }
        worldCenter.x /= building.base.size();
        worldCenter.y /= building.base.size();

        // Convert base to screen coordinates with footprint shrinking
        std::vector<ImVec2> base;
        base.reserve(building.base.size());
        for (const Vec2& point : building.base) {
            Vec2 shrunkPoint(
                worldCenter.x + (point.x - worldCenter.x) * 0.82f,
                worldCenter.y + (point.y - worldCenter.y) * 0.82f
            );
            Vec2 screenPoint = applyCamera(shrunkPoint, camera);
            base.push_back(ImVec2(screenPoint.x, screenPoint.y));
        }

        // 1. Drop shadow offset by height
        float shadowOffset = std::min(15.0f * z, building.height * 0.15f * z);
        std::vector<ImVec2> shadow;
        shadow.reserve(base.size());
        for (const ImVec2& p : base) {
            shadow.push_back(ImVec2(p.x + shadowOffset, p.y + shadowOffset));
        }
        drawList->AddConvexPolyFilled(
            shadow.data(),
            static_cast<int>(shadow.size()),
            IM_COL32(0, 0, 0, 95)
        );

        // Determine building styling
        int style = buildingIndex % 4;
        ImU32 roofColor;
        ImU32 borderColor;

        if (style == 0) {
            // Style 0: Dark slate roof (modern gravel)
            roofColor = IM_COL32(42, 46, 50, 255);
            borderColor = IM_COL32(80, 85, 90, 255);
        } else if (style == 1) {
            // Style 1: Light concrete roof
            roofColor = IM_COL32(185, 190, 195, 255);
            borderColor = IM_COL32(110, 115, 120, 255);
        } else if (style == 2) {
            // Style 2: Solar panel roof (dark navy blue)
            roofColor = IM_COL32(20, 35, 60, 255);
            borderColor = IM_COL32(60, 80, 110, 255);
        } else {
            // Style 3: Terracotta roof tiles
            roofColor = IM_COL32(200, 95, 65, 255);
            borderColor = IM_COL32(140, 60, 40, 255);
        }

        // 2. Draw Main Building Body (Roof)
        drawList->AddConvexPolyFilled(
            base.data(),
            static_cast<int>(base.size()),
            roofColor
        );

        // Draw Thick Border
        drawList->AddPolyline(
            base.data(),
            static_cast<int>(base.size()),
            borderColor,
            ImDrawFlags_Closed,
            2.5f * z
        );

        // 3. Draw Inner Shadow / Border Inset
        std::vector<ImVec2> insetBase;
        insetBase.reserve(base.size());
        ImVec2 center(0.0f, 0.0f);
        for (const ImVec2& p : base) {
            center.x += p.x;
            center.y += p.y;
        }
        center.x /= base.size();
        center.y /= base.size();

        for (const ImVec2& p : base) {
            insetBase.push_back(lerp(p, center, 0.08f));
        }

        drawList->AddPolyline(
            insetBase.data(),
            static_cast<int>(insetBase.size()),
            IM_COL32(0, 0, 0, 50),
            ImDrawFlags_Closed,
            1.0f * z
        );

        // 4. Roof Details
        if (z > 0.40f) {
            if (style == 2) {
                // Draw Solar Panel Grid Lines
                for (float t = 0.2f; t < 0.9f; t += 0.2f) {
                    ImVec2 startL = lerp(insetBase[0], insetBase[1], t);
                    ImVec2 endL = lerp(insetBase[3], insetBase[2], t);
                    drawList->AddLine(startL, endL, IM_COL32(100, 180, 255, 60), 1.0f * z);
                }
            } else if (building.height > 80.0f) {
                // Tall buildings get a Helipad
                float padRadius = 9.0f * z;
                drawList->AddCircleFilled(center, padRadius, IM_COL32(230, 40, 40, 225));
                drawList->AddCircle(center, padRadius, IM_COL32(255, 255, 255, 240), 0, 1.5f * z);
                // Draw "H"
                drawList->AddLine(
                    ImVec2(center.x - 4.5f * z, center.y - 5.0f * z),
                    ImVec2(center.x - 4.5f * z, center.y + 5.0f * z),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
                drawList->AddLine(
                    ImVec2(center.x + 4.5f * z, center.y - 5.0f * z),
                    ImVec2(center.x + 4.5f * z, center.y + 5.0f * z),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
                drawList->AddLine(
                    ImVec2(center.x - 4.5f * z, center.y),
                    ImVec2(center.x + 4.5f * z, center.y),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
            } else {
                // Small ventilation/machinery boxes
                ImVec2 box0 = lerp(insetBase[0], center, 0.4f);
                ImVec2 box2 = lerp(insetBase[0], center, 0.7f);
                ImVec2 box1(box2.x, box0.y);
                ImVec2 box3(box0.x, box2.y);
                ImVec2 ventBox[4] = { box0, box1, box2, box3 };

                // Vent Shadow
                ImVec2 ventShadow[4];
                for (int v = 0; v < 4; v++) {
                    ventShadow[v] = ImVec2(ventBox[v].x + 2.0f * z, ventBox[v].y + 2.0f * z);
                }
                drawList->AddConvexPolyFilled(ventShadow, 4, IM_COL32(0, 0, 0, 60));
                // Vent Body
                drawList->AddConvexPolyFilled(ventBox, 4, IM_COL32(100, 105, 110, 255));
                drawList->AddPolyline(ventBox, 4, IM_COL32(60, 60, 60, 255), ImDrawFlags_Closed, 1.0f * z);
            }
        }
    }
}

void Renderer::drawBuildingFills2_5D(
    const CityArea& area,
    const LiveContextEngine& liveContext,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float z = camera.getZoom();

    auto lerp = [](const ImVec2& a, const ImVec2& b, float t) {
        return ImVec2(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        );
    };

    // 1. Stage and depth-sort building indices back-to-front based on projection position
    std::vector<std::pair<float, size_t>> sortedBuildings;
    sortedBuildings.reserve(area.buildings.size());

    for (size_t idx = 0; idx < area.buildings.size(); ++idx) {
        const Building& b = area.buildings[idx];
        if (b.base.size() < 3) continue;

        // Compute centroid
        Vec2 center(0.0f, 0.0f);
        for (const Vec2& pt : b.base) {
            center.x += pt.x;
            center.y += pt.y;
        }
        center.x /= b.base.size();
        center.y /= b.base.size();

        // Depth metric: back-to-front sorting.
        // points with smaller x+y are further away.
        float depth = center.x + center.y;
        sortedBuildings.push_back({depth, idx});
    }

    std::sort(sortedBuildings.begin(), sortedBuildings.end(),
              [](const std::pair<float, size_t>& a, const std::pair<float, size_t>& b) {
                  return a.first < b.first;
              });

    for (const auto& pair : sortedBuildings) {
        size_t origIdx = pair.second;
        const Building& building = area.buildings[origIdx];
        int buildingIndex = static_cast<int>(origIdx) + 1;

        // Compute centroid of this building for shrinking
        Vec2 center(0.0f, 0.0f);
        for (const Vec2& pt : building.base) {
            center.x += pt.x;
            center.y += pt.y;
        }
        center.x /= building.base.size();
        center.y /= building.base.size();

        std::vector<ImVec2> base;
        std::vector<ImVec2> top;
        base.reserve(building.base.size());
        top.reserve(building.base.size());

        for (const Vec2& point : building.base) {
            // Shrink footprint towards center by 18% (scale factor 0.82)
            Vec2 shrunkPoint(
                center.x + (point.x - center.x) * 0.82f,
                center.y + (point.y - center.y) * 0.82f
            );

            Vec2 projected = Projection2_5D::projectPoint(shrunkPoint);
            Vec2 projectedTop = Projection2_5D::shiftUp(projected, building.height);

            projected = applyCamera(projected, camera);
            projectedTop = applyCamera(projectedTop, camera);

            base.push_back(ImVec2(projected.x, projected.y));
            top.push_back(ImVec2(projectedTop.x, projectedTop.y));
        }

        // 1. Draw Base Shadow
        std::vector<ImVec2> shadow;
        shadow.reserve(base.size());
        for (const ImVec2& p : base) {
            shadow.push_back(ImVec2(
                p.x + 25.0f * z,
                p.y + 18.0f * z
            ));
        }
        drawList->AddConvexPolyFilled(
            shadow.data(),
            static_cast<int>(shadow.size()),
            IM_COL32(0, 0, 0, 75)
        );

        // Determine building material and color palette
        int material = buildingIndex % 4;
        ImU32 bodyColor;
        ImU32 roofColor;
        ImU32 lineWhite = IM_COL32(255, 255, 255, 65);
        ImU32 lineDark = IM_COL32(0, 0, 0, 60);

        if (material == 0) {
            // Glass Skyscraper: Deep navy blue with bright cyan roof
            bodyColor = IM_COL32(20, 60, 100, 240);
            roofColor = IM_COL32(40, 150, 210, 255);
        } else if (material == 1) {
            // Concrete / Brutalist: Warm sand/gray concrete
            bodyColor = IM_COL32(145, 140, 130, 240);
            roofColor = IM_COL32(175, 170, 160, 255);
        } else if (material == 2) {
            // Terracotta / Brick: Deep terracotta orange
            bodyColor = IM_COL32(165, 70, 45, 240);
            roofColor = IM_COL32(200, 100, 75, 255);
        } else {
            // Modern Tech Obsidian: Dark slate grey
            bodyColor = IM_COL32(45, 50, 55, 240);
            roofColor = IM_COL32(75, 80, 85, 255);
        }

        // Helper lambda to adjust color brightness
        auto adjustColor = [](ImU32 col, float multiplier) -> ImU32 {
            int r = (col & 0xFF);
            int g = ((col >> 8) & 0xFF);
            int b = ((col >> 16) & 0xFF);
            int a = ((col >> 24) & 0xFF);

            r = std::max(0, std::min(255, static_cast<int>(r * multiplier)));
            g = std::max(0, std::min(255, static_cast<int>(g * multiplier)));
            b = std::max(0, std::min(255, static_cast<int>(b * multiplier)));

            return IM_COL32(r, g, b, a);
        };

        // 2. Draw Side Faces (Walls) with Perspective Shading & Textures
        for (size_t i = 0; i < base.size(); i++) {
            size_t next = (i + 1) % base.size();

            ImVec2 sideFace[4] = {
                base[i],
                base[next],
                top[next],
                top[i]
            };

            // Calculate face orientation relative to light source
            float shadingMultiplier = (i % 2 == 0) ? 1.15f : 0.85f;
            ImU32 faceColor = adjustColor(bodyColor, shadingMultiplier);

            drawList->AddConvexPolyFilled(sideFace, 4, faceColor);

            // Draw facade structural lines / panel dividers (realism detail)
            if (z > 0.40f) {
                if (material == 0) {
                    // Glass grid columns
                    for (int c = 1; c < 5; c++) {
                        float tx = static_cast<float>(c) / 5.0f;
                        drawList->AddLine(
                            lerp(sideFace[0], sideFace[1], tx),
                            lerp(sideFace[3], sideFace[2], tx),
                            IM_COL32(120, 200, 240, 60),
                            0.8f * z
                        );
                    }
                    // Glass grid rows
                    for (int r = 1; r < 6; r++) {
                        float ty = static_cast<float>(r) / 6.0f;
                        drawList->AddLine(
                            lerp(sideFace[0], sideFace[3], ty),
                            lerp(sideFace[1], sideFace[2], ty),
                            IM_COL32(120, 200, 240, 60),
                            0.8f * z
                        );
                    }
                } else if (material == 1) {
                    // Concrete horizontal seam lines
                    for (int r = 1; r < 4; r++) {
                        float ty = static_cast<float>(r) / 4.0f;
                        drawList->AddLine(
                            lerp(sideFace[0], sideFace[3], ty),
                            lerp(sideFace[1], sideFace[2], ty),
                            IM_COL32(75, 70, 65, 80),
                            1.0f * z
                        );
                    }
                } else if (material == 2) {
                    // Brick row lines
                    for (int r = 1; r < 12; r++) {
                        float ty = static_cast<float>(r) / 12.0f;
                        drawList->AddLine(
                            lerp(sideFace[0], sideFace[3], ty),
                            lerp(sideFace[1], sideFace[2], ty),
                            IM_COL32(100, 40, 25, 70),
                            0.7f * z
                        );
                    }
                } else {
                    // Modern obsidian panel rows
                    for (int r = 1; r < 5; r++) {
                        float ty = static_cast<float>(r) / 5.0f;
                        drawList->AddLine(
                            lerp(sideFace[0], sideFace[3], ty),
                            lerp(sideFace[1], sideFace[2], ty),
                            IM_COL32(20, 22, 25, 120),
                            1.2f * z
                        );
                    }
                }
            }
            
            // Draw face border outline (white highlights on bright side, dark lines on dark side)
            ImU32 outlineColor = (i % 2 == 0) ? lineWhite : lineDark;
            drawList->AddPolyline(sideFace, 4, outlineColor, ImDrawFlags_Closed, 1.0f * z);

            // 3. Draw Isometric Windows on Walls (if zoomed in enough)
            if (z > 0.45f) {
                // Determine windows count based on building height/width
                int num_rows = 3 + static_cast<int>(building.height / 30.0f);
                int num_cols = 4;
                bool isNight = liveContext.isNightMode();

                for (int r = 0; r < num_rows; r++) {
                    for (int c = 0; c < num_cols; c++) {
                        float tx0 = (c + 0.20f) / num_cols;
                        float tx1 = (c + 0.80f) / num_cols;
                        float ty0 = (r + 0.20f) / num_rows;
                        float ty1 = (r + 0.80f) / num_rows;

                        auto getFacePoint = [&](float tx, float ty) {
                            ImVec2 bot = lerp(sideFace[0], sideFace[1], tx);
                            ImVec2 tp = lerp(sideFace[3], sideFace[2], tx);
                            return lerp(bot, tp, ty);
                        };

                        ImVec2 w0 = getFacePoint(tx0, ty0);
                        ImVec2 w1 = getFacePoint(tx1, ty0);
                        ImVec2 w2 = getFacePoint(tx1, ty1);
                        ImVec2 w3 = getFacePoint(tx0, ty1);

                        ImVec2 windowPane[4] = { w0, w1, w2, w3 };

                        // Dynamic windows glow color (night lit vs unlit, day cyan)
                        ImU32 windowColor;
                        if (isNight) {
                            int hash = (r * 13 + c * 37 + buildingIndex * 97 + static_cast<int>(i) * 11) % 100;
                            if (hash < 25) {
                                windowColor = IM_COL32(255, 215, 80, 220); // Warm Lit
                            } else if (hash < 40) {
                                windowColor = IM_COL32(170, 240, 255, 190); // Office Lit
                            } else {
                                windowColor = IM_COL32(22, 26, 35, 240); // Dark
                            }
                        } else {
                            windowColor = IM_COL32(185, 235, 255, 185); // Daytime sky reflection
                        }

                        drawList->AddConvexPolyFilled(windowPane, 4, windowColor);
                        
                        // Thin window frame
                        drawList->AddPolyline(windowPane, 4, IM_COL32(35, 38, 42, 100), ImDrawFlags_Closed, 0.7f * z);
                    }
                }
            }
        }

        // 4. Draw Roof Face
        drawList->AddConvexPolyFilled(
            top.data(),
            static_cast<int>(top.size()),
            roofColor
        );
        drawList->AddPolyline(
            top.data(),
            static_cast<int>(top.size()),
            lineWhite,
            ImDrawFlags_Closed,
            1.5f * z
        );

        // Draw Inset roof border / Parapet wall
        ImVec2 roofCenter(0.0f, 0.0f);
        for (const ImVec2& p : top) {
            roofCenter.x += p.x;
            roofCenter.y += p.y;
        }
        roofCenter.x /= top.size();
        roofCenter.y /= top.size();

        std::vector<ImVec2> insetTop;
        insetTop.reserve(top.size());
        for (const ImVec2& p : top) {
            insetTop.push_back(lerp(p, roofCenter, 0.08f));
        }

        drawList->AddPolyline(
            insetTop.data(),
            static_cast<int>(insetTop.size()),
            adjustColor(roofColor, 0.80f),
            ImDrawFlags_Closed,
            1.2f * z
        );

        // 5. Draw Roof Solar Panels (for obsidian buildings) or Helipad (for tall buildings)
        if (z > 0.40f) {
            if (material == 3) {
                // Solar grid in the center
                std::vector<ImVec2> solarBase;
                solarBase.reserve(top.size());
                for (const ImVec2& p : top) {
                    solarBase.push_back(lerp(p, roofCenter, 0.15f));
                }
                drawList->AddConvexPolyFilled(solarBase.data(), static_cast<int>(solarBase.size()), IM_COL32(15, 25, 45, 245));
                drawList->AddPolyline(solarBase.data(), static_cast<int>(solarBase.size()), IM_COL32(50, 90, 150, 150), ImDrawFlags_Closed, 1.0f * z);
                // Grid divides
                for (float t = 0.25f; t < 0.9f; t += 0.25f) {
                    drawList->AddLine(lerp(solarBase[0], solarBase[1], t), lerp(solarBase[3], solarBase[2], t), IM_COL32(60, 100, 160, 160), 0.8f * z);
                    drawList->AddLine(lerp(solarBase[0], solarBase[3], t), lerp(solarBase[1], solarBase[2], t), IM_COL32(60, 100, 160, 160), 0.8f * z);
                }
            } else if (building.height > 80.0f) {
                // Helipad
                float padRadius = 9.0f * z;
                drawList->AddCircleFilled(roofCenter, padRadius, IM_COL32(230, 40, 40, 225));
                drawList->AddCircle(roofCenter, padRadius, IM_COL32(255, 255, 255, 240), 0, 1.5f * z);
                
                // "H" letter
                drawList->AddLine(
                    ImVec2(roofCenter.x - 4.5f * z, roofCenter.y - 5.0f * z),
                    ImVec2(roofCenter.x - 4.5f * z, roofCenter.y + 5.0f * z),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
                drawList->AddLine(
                    ImVec2(roofCenter.x + 4.5f * z, roofCenter.y - 5.0f * z),
                    ImVec2(roofCenter.x + 4.5f * z, roofCenter.y + 5.0f * z),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
                drawList->AddLine(
                    ImVec2(roofCenter.x - 4.5f * z, roofCenter.y),
                    ImVec2(roofCenter.x + 4.5f * z, roofCenter.y),
                    IM_COL32(255, 255, 255, 255),
                    2.0f * z
                );
            }

            // Draw HVAC machinery box on the side of the roof
            ImVec2 hvacBase0 = ImVec2(roofCenter.x + 8.0f * z, roofCenter.y - 1.0f * z);
            ImVec2 hvacBase1 = ImVec2(roofCenter.x + 13.0f * z, roofCenter.y - 3.0f * z);
            ImVec2 hvacBase2 = ImVec2(roofCenter.x + 11.0f * z, roofCenter.y - 6.0f * z);
            ImVec2 hvacBase3 = ImVec2(roofCenter.x + 6.0f * z, roofCenter.y - 4.0f * z);
            ImVec2 hvacTop0(hvacBase0.x, hvacBase0.y - 5.0f * z);
            ImVec2 hvacTop1(hvacBase1.x, hvacBase1.y - 5.0f * z);
            ImVec2 hvacTop2(hvacBase2.x, hvacBase2.y - 5.0f * z);
            ImVec2 hvacTop3(hvacBase3.x, hvacBase3.y - 5.0f * z);

            ImVec2 hL[4] = { hvacBase0, hvacBase1, hvacTop1, hvacTop0 };
            ImVec2 hR[4] = { hvacBase1, hvacBase2, hvacTop2, hvacTop1 };
            ImVec2 hT[4] = { hvacTop0, hvacTop1, hvacTop2, hvacTop3 };

            drawList->AddConvexPolyFilled(hL, 4, IM_COL32(110, 110, 115, 255));
            drawList->AddConvexPolyFilled(hR, 4, IM_COL32(80, 80, 85, 255));
            drawList->AddConvexPolyFilled(hT, 4, IM_COL32(140, 140, 145, 255));
            drawList->AddPolyline(hL, 4, IM_COL32(50, 50, 55, 255), ImDrawFlags_Closed, 0.8f * z);
            drawList->AddPolyline(hR, 4, IM_COL32(50, 50, 55, 255), ImDrawFlags_Closed, 0.8f * z);

            // Tall buildings get a warning beacon light on a mast
            if (building.height > 80.0f) {
                ImVec2 mastTip = ImVec2(roofCenter.x, roofCenter.y - 22.0f * z);
                drawList->AddLine(roofCenter, mastTip, IM_COL32(150, 150, 155, 255), 1.2f * z);
                
                bool beaconOn = (static_cast<int>(ImGui::GetTime() * 2.5) % 2 == 0);
                if (beaconOn || liveContext.isNightMode()) {
                    drawList->AddCircleFilled(mastTip, 2.5f * z, IM_COL32(255, 40, 40, 255));
                    if (liveContext.isNightMode()) {
                        drawList->AddCircleFilled(mastTip, 7.0f * z, IM_COL32(255, 40, 40, 80)); // Soft Night Glow
                    }
                }
            }
        }
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

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // In X-Ray mode use an inset clip window so clipping is visually obvious.
    int clipMargin = xrayMode ? 80 : 0;
    int clipX1 = clipMargin;
    int clipY1 = clipMargin;
    int clipX2 = static_cast<int>(displaySize.x) - clipMargin;
    int clipY2 = static_cast<int>(displaySize.y) - clipMargin;

    auto drawLineClipped = [&](Vec2 a, Vec2 b, Color color, bool useDDA) {
        int ax = static_cast<int>(a.x), ay = static_cast<int>(a.y);
        int bx = static_cast<int>(b.x), by = static_cast<int>(b.y);
        int cx1, cy1, cx2, cy2;

        if (!ClippingAlgorithms::cohenSutherland(ax, ay, bx, by, clipX1, clipY1, clipX2, clipY2, cx1, cy1, cx2, cy2)) {
            return;
        }

        if (useDDA) {
            LineAlgorithms::drawLineDDA(pixelBuffer, cx1, cy1, cx2, cy2, color);
        } else {
            LineAlgorithms::drawLineBresenham(pixelBuffer, cx1, cy1, cx2, cy2, color);
        }
    };

    Color roadColor(0.75f, 0.75f, 0.75f, 1.0f);
    Color buildingColor = xrayMode ? Color(0.2f, 0.8f, 1.0f, 1.0f) : Color(0.12f, 0.12f, 0.15f, 0.28f);
    Color routeColor(1.0f, 0.9f, 0.1f, 1.0f);
    Color crossingColor(1.0f, 1.0f, 1.0f, 1.0f);
    bool useDDA = (selectedLineAlgorithm == 0);

    for (const Road& road : area.roads) {
        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 a = transformForView(road.points[i], isometricMode);
            Vec2 b = transformForView(road.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            drawLineClipped(a, b, roadColor, useDDA);
        }
    }

    if (xrayMode) {
        for (const VehicleRoute& route : area.routes) {
            for (size_t i = 0; i + 1 < route.points.size(); i++) {
                Vec2 a = transformForView(route.points[i], isometricMode);
                Vec2 b = transformForView(route.points[i + 1], isometricMode);

                a = applyCamera(a, camera);
                b = applyCamera(b, camera);

                drawLineClipped(a, b, routeColor, true);
            }
        }
    }

    for (const Building& building : area.buildings) {
        if (building.base.size() < 3) {
            continue;
        }

        // Compute centroid
        Vec2 center(0.0f, 0.0f);
        for (const Vec2& pt : building.base) {
            center.x += pt.x;
            center.y += pt.y;
        }
        center.x /= building.base.size();
        center.y /= building.base.size();

        if (!isometricMode) {
            for (size_t i = 0; i < building.base.size(); i++) {
                Vec2 ptA = building.base[i];
                Vec2 ptB = building.base[(i + 1) % building.base.size()];

                Vec2 shrunkA(
                    center.x + (ptA.x - center.x) * 0.82f,
                    center.y + (ptA.y - center.y) * 0.82f
                );
                Vec2 shrunkB(
                    center.x + (ptB.x - center.x) * 0.82f,
                    center.y + (ptB.y - center.y) * 0.82f
                );

                Vec2 a = applyCamera(shrunkA, camera);
                Vec2 b = applyCamera(shrunkB, camera);

                drawLineClipped(a, b, buildingColor, false);
            }
        } else {
            std::vector<Vec2> base;
            std::vector<Vec2> top;

            for (const Vec2& point : building.base) {
                // Shrink footprint towards center by 18% (scale factor 0.82)
                Vec2 shrunkPoint(
                    center.x + (point.x - center.x) * 0.82f,
                    center.y + (point.y - center.y) * 0.82f
                );

                Vec2 projected = Projection2_5D::projectPoint(shrunkPoint);
                Vec2 projectedTop = Projection2_5D::shiftUp(projected, building.height);

                projected = applyCamera(projected, camera);
                projectedTop = applyCamera(projectedTop, camera);

                base.push_back(projected);
                top.push_back(projectedTop);
            }

            for (size_t i = 0; i < base.size(); i++) {
                size_t next = (i + 1) % base.size();

                drawLineClipped(base[i], base[next], buildingColor, false);
                drawLineClipped(top[i], top[next], buildingColor, false);
                drawLineClipped(base[i], top[i], buildingColor, false);
            }
        }
    }

    for (const PedestrianCrossing& crossing : area.crossings) {
        for (size_t i = 0; i + 1 < crossing.points.size(); i++) {
            Vec2 a = transformForView(crossing.points[i], isometricMode);
            Vec2 b = transformForView(crossing.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            drawLineClipped(a, b, crossingColor, false);
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

    auto adjustColor = [](ImU32 col, float multiplier) -> ImU32 {
        int r = (col & 0xFF);
        int g = ((col >> 8) & 0xFF);
        int b = ((col >> 16) & 0xFF);
        int a = ((col >> 24) & 0xFF);

        r = std::max(0, std::min(255, static_cast<int>(r * multiplier)));
        g = std::max(0, std::min(255, static_cast<int>(g * multiplier)));
        b = std::max(0, std::min(255, static_cast<int>(b * multiplier)));

        return IM_COL32(r, g, b, a);
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

        ImVec2 p0 = screenVertices[0]; // rear-left
        ImVec2 p1 = screenVertices[1]; // front-left
        ImVec2 p2 = screenVertices[2]; // front-right
        ImVec2 p3 = screenVertices[3]; // rear-right

        // Determine vehicle type and dimensional profile
        int vType = vehicleIndex % 5;
        float hVal = 8.0f; // Default height
        ImU32 baseColor;

        switch (vType) {
            case 0: // Sedan/Standard car (Orange)
                hVal = 9.0f;
                baseColor = IM_COL32(235, 110, 30, 245);
                break;
            case 1: // Bus (Blue)
                hVal = 17.0f; // Tall
                baseColor = IM_COL32(40, 130, 240, 245);
                break;
            case 2: // SUV/Truck (White/Silver)
                hVal = 13.0f;
                baseColor = IM_COL32(220, 220, 225, 245);
                break;
            case 3: // Cab/Taxi (Yellow)
                hVal = 9.0f;
                baseColor = IM_COL32(245, 205, 30, 245);
                break;
            default: // Sports car (Red)
                hVal = 6.5f; // Low profile
                baseColor = IM_COL32(230, 40, 70, 245);
                break;
        }
        float H = hVal * z;

        float opacity = vehicle.getOpacity();
        auto getFadedColor = [adjustColor, opacity](ImU32 col, float multiplier) -> ImU32 {
            ImU32 base = adjustColor(col, multiplier);
            int r = (base & 0xFF);
            int g = ((base >> 8) & 0xFF);
            int b = ((base >> 16) & 0xFF);
            int a = static_cast<int>(((base >> 24) & 0xFF) * opacity);
            return IM_COL32(r, g, b, a);
        };

        if (isometricMode) {
            // Shadow under car base
            std::vector<ImVec2> shadowVertices;
            for (const ImVec2& p : screenVertices) {
                shadowVertices.push_back(ImVec2(p.x + 4.0f * z, p.y + 6.0f * z));
            }
            drawList->AddConvexPolyFilled(shadowVertices.data(), 4, getFadedColor(IM_COL32(0, 0, 0, 75), 1.0f));

            if (vType == 1) {
                // --- BUS SHAPE (Single tall box with multiple windows) ---
                ImVec2 t0(p0.x, p0.y - H * 0.6f);
                ImVec2 t1(p1.x, p1.y - H * 0.6f);
                ImVec2 t2(p2.x, p2.y - H * 0.6f);
                ImVec2 t3(p3.x, p3.y - H * 0.6f);

                // Rear Face
                ImVec2 rearFace[4] = { p0, p3, t3, t0 };
                drawList->AddConvexPolyFilled(rearFace, 4, getFadedColor(baseColor, 0.70f));
                drawList->AddPolyline(rearFace, 4, getFadedColor(IM_COL32(0, 0, 0, 40), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Left Face (Side)
                ImVec2 leftFace[4] = { p0, p1, t1, t0 };
                drawList->AddConvexPolyFilled(leftFace, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(leftFace, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Right Face (Side)
                ImVec2 rightFace[4] = { p3, p2, t2, t3 };
                drawList->AddConvexPolyFilled(rightFace, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(rightFace, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Front Face
                ImVec2 frontFace[4] = { p1, p2, t2, t1 };
                drawList->AddConvexPolyFilled(frontFace, 4, getFadedColor(baseColor, 1.05f));
                drawList->AddPolyline(frontFace, 4, getFadedColor(IM_COL32(255, 255, 255, 80), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Top Roof Face
                ImVec2 topFace[4] = { t0, t1, t2, t3 };
                drawList->AddConvexPolyFilled(topFace, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(topFace, 4, getFadedColor(IM_COL32(255, 255, 255, 120), 1.0f), ImDrawFlags_Closed, 1.5f * z);

                // Bus side passenger windows (5 window panes on each side)
                for (int iw = 0; iw < 5; iw++) {
                    float tStart = 0.10f + iw * 0.17f;
                    float tEnd = tStart + 0.12f;

                    // Left Side Windows
                    ImVec2 wTL = lerp(t0, t1, tStart);
                    ImVec2 wTR = lerp(t0, t1, tEnd);
                    ImVec2 wBL = lerp(p0, p1, tStart);
                    ImVec2 wBR = lerp(p0, p1, tEnd);

                    wBL.y -= H * 0.35f * 0.6f;
                    wBR.y -= H * 0.35f * 0.6f;
                    wTL.y -= H * 0.10f * 0.6f;
                    wTR.y -= H * 0.10f * 0.6f;

                    ImVec2 winL[4] = { wBL, wBR, wTR, wTL };
                    drawList->AddConvexPolyFilled(winL, 4, getFadedColor(IM_COL32(20, 30, 45, 220), 1.0f));
                    drawList->AddPolyline(winL, 4, getFadedColor(IM_COL32(255, 255, 255, 50), 1.0f), ImDrawFlags_Closed, 0.8f * z);

                    // Right Side Windows
                    ImVec2 wTL_R = lerp(t3, t2, tStart);
                    ImVec2 wTR_R = lerp(t3, t2, tEnd);
                    ImVec2 wBL_R = lerp(p3, p2, tStart);
                    ImVec2 wBR_R = lerp(p3, p2, tEnd);

                    wBL_R.y -= H * 0.35f * 0.6f;
                    wBR_R.y -= H * 0.35f * 0.6f;
                    wTL_R.y -= H * 0.10f * 0.6f;
                    wTR_R.y -= H * 0.10f * 0.6f;

                    ImVec2 winR[4] = { wBL_R, wBR_R, wTR_R, wTL_R };
                    drawList->AddConvexPolyFilled(winR, 4, getFadedColor(IM_COL32(20, 30, 45, 220), 1.0f));
                    drawList->AddPolyline(winR, 4, getFadedColor(IM_COL32(255, 255, 255, 50), 1.0f), ImDrawFlags_Closed, 0.8f * z);
                }
            }
            else if (vType == 2) {
                // --- SUV / PICKUP TRUCK SHAPE (Three-box layout: Open Bed, Cabin, Hood) ---
                // Bed: 0% to 42%, Cabin: 42% to 76%, Hood: 76% to 100%
                ImVec2 mBedL = lerp(p0, p1, 0.42f);
                ImVec2 mBedR = lerp(p3, p2, 0.42f);
                ImVec2 mHoodL = lerp(p0, p1, 0.76f);
                ImVec2 mHoodR = lerp(p3, p2, 0.76f);

                float H_bed = H * 0.45f;
                float H_cabin = H;
                float H_hood = H * 0.55f;

                // Elevated corners:
                // Bed top
                ImVec2 tBedRearL(p0.x, p0.y - H_bed * 0.6f);
                ImVec2 tBedRearR(p3.x, p3.y - H_bed * 0.6f);
                ImVec2 tBedFrontL(mBedL.x, mBedL.y - H_bed * 0.6f);
                ImVec2 tBedFrontR(mBedR.x, mBedR.y - H_bed * 0.6f);

                // Hood top
                ImVec2 tHoodRearL(mHoodL.x, mHoodL.y - H_hood * 0.6f);
                ImVec2 tHoodRearR(mHoodR.x, mHoodR.y - H_hood * 0.6f);
                ImVec2 tHoodFrontL(p1.x, p1.y - H_hood * 0.6f);
                ImVec2 tHoodFrontR(p2.x, p2.y - H_hood * 0.6f);

                // Cabin top
                ImVec2 tCabinRearL(mBedL.x, mBedL.y - H_cabin * 0.6f);
                ImVec2 tCabinRearR(mBedR.x, mBedR.y - H_cabin * 0.6f);
                ImVec2 tCabinFrontL(mHoodL.x, mHoodL.y - H_cabin * 0.6f);
                ImVec2 tCabinFrontR(mHoodR.x, mHoodR.y - H_cabin * 0.6f);

                // 1. Draw Bed Block
                // Rear bumper face
                ImVec2 bedRear[4] = { p0, p3, tBedRearR, tBedRearL };
                drawList->AddConvexPolyFilled(bedRear, 4, getFadedColor(baseColor, 0.70f));
                drawList->AddPolyline(bedRear, 4, getFadedColor(IM_COL32(0, 0, 0, 40), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Bed sides
                ImVec2 bedLeft[4] = { p0, mBedL, tBedFrontL, tBedRearL };
                drawList->AddConvexPolyFilled(bedLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(bedLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 bedRight[4] = { p3, mBedR, tBedFrontR, tBedRearR };
                drawList->AddConvexPolyFilled(bedRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(bedRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Bed top face (open cargo area colored darker)
                ImVec2 bedTop[4] = { tBedRearL, tBedRearR, tBedFrontR, tBedFrontL };
                drawList->AddConvexPolyFilled(bedTop, 4, getFadedColor(baseColor, 0.60f));
                drawList->AddPolyline(bedTop, 4, getFadedColor(IM_COL32(0, 0, 0, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // 2. Draw Hood Block
                // Front bumper face
                ImVec2 hoodFront[4] = { p1, p2, tHoodFrontR, tHoodFrontL };
                drawList->AddConvexPolyFilled(hoodFront, 4, getFadedColor(baseColor, 1.05f));
                drawList->AddPolyline(hoodFront, 4, getFadedColor(IM_COL32(255, 255, 255, 80), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Hood sides
                ImVec2 hoodLeft[4] = { mHoodL, p1, tHoodFrontL, tHoodRearL };
                drawList->AddConvexPolyFilled(hoodLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(hoodLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 hoodRight[4] = { mHoodR, p2, tHoodFrontR, tHoodRearR };
                drawList->AddConvexPolyFilled(hoodRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(hoodRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Hood top face
                ImVec2 hoodTop[4] = { tHoodRearL, tHoodRearR, tHoodFrontR, tHoodFrontL };
                drawList->AddConvexPolyFilled(hoodTop, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(hoodTop, 4, getFadedColor(IM_COL32(255, 255, 255, 70), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // 3. Draw Cabin Block
                // Cabin sides
                ImVec2 cabLeft[4] = { mBedL, mHoodL, tCabinFrontL, tCabinRearL };
                drawList->AddConvexPolyFilled(cabLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(cabLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 cabRight[4] = { mBedR, mHoodR, tCabinFrontR, tCabinRearR };
                drawList->AddConvexPolyFilled(cabRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(cabRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Windshield (hood to cabin top)
                ImVec2 frontWindshield[4] = { tHoodRearL, tHoodRearR, tCabinFrontR, tCabinFrontL };
                drawList->AddConvexPolyFilled(frontWindshield, 4, getFadedColor(IM_COL32(110, 205, 255, 180), 1.0f));
                drawList->AddPolyline(frontWindshield, 4, getFadedColor(IM_COL32(180, 230, 255, 180), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Back window (bed to cabin top)
                ImVec2 backWindshield[4] = { tBedFrontL, tBedFrontR, tCabinRearR, tCabinRearL };
                drawList->AddConvexPolyFilled(backWindshield, 4, getFadedColor(IM_COL32(110, 205, 255, 180), 1.0f));
                drawList->AddPolyline(backWindshield, 4, getFadedColor(IM_COL32(180, 230, 255, 180), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Cabin Roof
                ImVec2 roofTop[4] = { tCabinRearL, tCabinRearR, tCabinFrontR, tCabinFrontL };
                drawList->AddConvexPolyFilled(roofTop, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(roofTop, 4, getFadedColor(IM_COL32(255, 255, 255, 120), 1.0f), ImDrawFlags_Closed, 1.5f * z);

                // Side windows on Left Cabin
                ImVec2 wC0 = lerp(tCabinRearL, tCabinFrontL, 0.20f);
                ImVec2 wC1 = lerp(tCabinRearL, tCabinFrontL, 0.80f);
                ImVec2 wCB0 = lerp(mBedL, mHoodL, 0.20f);
                ImVec2 wCB1 = lerp(mBedL, mHoodL, 0.80f);

                ImVec2 sWinL[4] = {
                    lerp(wCB0, wC0, 0.35f),
                    lerp(wCB1, wC1, 0.35f),
                    wC1,
                    wC0
                };
                drawList->AddConvexPolyFilled(sWinL, 4, getFadedColor(IM_COL32(20, 30, 45, 200), 1.0f));

                // Side windows on Right Cabin
                ImVec2 wCR0 = lerp(tCabinRearR, tCabinFrontR, 0.20f);
                ImVec2 wCR1 = lerp(tCabinRearR, tCabinFrontR, 0.80f);
                ImVec2 wCBR0 = lerp(mBedR, mHoodR, 0.20f);
                ImVec2 wCBR1 = lerp(mBedR, mHoodR, 0.80f);

                ImVec2 sWinR[4] = {
                    lerp(wCBR0, wCR0, 0.35f),
                    lerp(wCBR1, wCR1, 0.35f),
                    wCR1,
                    wCR0
                };
                drawList->AddConvexPolyFilled(sWinR, 4, getFadedColor(IM_COL32(20, 30, 45, 200), 1.0f));
            }
            else {
                // --- PASSENGER CARS / SEDANS (Type 0, 3, 4 - Realistic 3-Box Shape) ---
                // Dividers along length: Trunk = 0% to 24%, Cabin = 24% to 72%, Hood = 72% to 100%
                ImVec2 mTrunkL = lerp(p0, p1, 0.24f);
                ImVec2 mTrunkR = lerp(p3, p2, 0.24f);
                ImVec2 mHoodL = lerp(p0, p1, 0.72f);
                ImVec2 mHoodR = lerp(p3, p2, 0.72f);

                float H_hood = H * 0.52f;
                float H_cabin = H;
                float H_trunk = H * 0.58f;

                // Hood top coordinates
                ImVec2 tHoodFrontL(p1.x, p1.y - H_hood * 0.6f);
                ImVec2 tHoodFrontR(p2.x, p2.y - H_hood * 0.6f);
                ImVec2 tHoodRearL(mHoodL.x, mHoodL.y - H_hood * 0.6f);
                ImVec2 tHoodRearR(mHoodR.x, mHoodR.y - H_hood * 0.6f);

                // Trunk top coordinates
                ImVec2 tTrunkRearL(p0.x, p0.y - H_trunk * 0.6f);
                ImVec2 tTrunkRearR(p3.x, p3.y - H_trunk * 0.6f);
                ImVec2 tTrunkFrontL(mTrunkL.x, mTrunkL.y - H_trunk * 0.6f);
                ImVec2 tTrunkFrontR(mTrunkR.x, mTrunkR.y - H_trunk * 0.6f);

                // Cabin top coordinates
                ImVec2 tCabinRearL(mTrunkL.x, mTrunkL.y - H_cabin * 0.6f);
                ImVec2 tCabinRearR(mTrunkR.x, mTrunkR.y - H_cabin * 0.6f);
                ImVec2 tCabinFrontL(mHoodL.x, mHoodL.y - H_cabin * 0.6f);
                ImVec2 tCabinFrontR(mHoodR.x, mHoodR.y - H_cabin * 0.6f);

                // 1. Draw Trunk Block
                // Rear bumper face
                ImVec2 trunkRear[4] = { p0, p3, tTrunkRearR, tTrunkRearL };
                drawList->AddConvexPolyFilled(trunkRear, 4, getFadedColor(baseColor, 0.70f));
                drawList->AddPolyline(trunkRear, 4, getFadedColor(IM_COL32(0, 0, 0, 40), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Trunk sides
                ImVec2 trunkLeft[4] = { p0, mTrunkL, tTrunkFrontL, tTrunkRearL };
                drawList->AddConvexPolyFilled(trunkLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(trunkLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 trunkRight[4] = { p3, mTrunkR, tTrunkFrontR, tTrunkRearR };
                drawList->AddConvexPolyFilled(trunkRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(trunkRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Trunk top face
                ImVec2 trunkTop[4] = { tTrunkRearL, tTrunkRearR, tTrunkFrontR, tTrunkFrontL };
                drawList->AddConvexPolyFilled(trunkTop, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(trunkTop, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // 2. Draw Hood Block
                // Front bumper face
                ImVec2 hoodFront[4] = { p1, p2, tHoodFrontR, tHoodFrontL };
                drawList->AddConvexPolyFilled(hoodFront, 4, getFadedColor(baseColor, 1.05f));
                drawList->AddPolyline(hoodFront, 4, getFadedColor(IM_COL32(255, 255, 255, 80), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Hood sides
                ImVec2 hoodLeft[4] = { mHoodL, p1, tHoodFrontL, tHoodRearL };
                drawList->AddConvexPolyFilled(hoodLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(hoodLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 hoodRight[4] = { mHoodR, p2, tHoodFrontR, tHoodRearR };
                drawList->AddConvexPolyFilled(hoodRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(hoodRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Hood top face
                ImVec2 hoodTop[4] = { tHoodRearL, tHoodRearR, tHoodFrontR, tHoodFrontL };
                drawList->AddConvexPolyFilled(hoodTop, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(hoodTop, 4, getFadedColor(IM_COL32(255, 255, 255, 70), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // 3. Draw Elevated Cabin block
                // Cabin sides
                ImVec2 cabLeft[4] = { mTrunkL, mHoodL, tCabinFrontL, tCabinRearL };
                drawList->AddConvexPolyFilled(cabLeft, 4, getFadedColor(baseColor, 1.15f));
                drawList->AddPolyline(cabLeft, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                ImVec2 cabRight[4] = { mTrunkR, mHoodR, tCabinFrontR, tCabinRearR };
                drawList->AddConvexPolyFilled(cabRight, 4, getFadedColor(baseColor, 0.85f));
                drawList->AddPolyline(cabRight, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Slanted front windshield
                ImVec2 frontWindshield[4] = { tHoodRearL, tHoodRearR, tCabinFrontR, tCabinFrontL };
                drawList->AddConvexPolyFilled(frontWindshield, 4, getFadedColor(IM_COL32(110, 205, 255, 180), 1.0f));
                drawList->AddPolyline(frontWindshield, 4, getFadedColor(IM_COL32(180, 230, 255, 180), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Slanted rear windshield
                ImVec2 rearWindshield[4] = { tTrunkFrontL, tTrunkFrontR, tCabinRearR, tCabinRearL };
                drawList->AddConvexPolyFilled(rearWindshield, 4, getFadedColor(IM_COL32(110, 205, 255, 180), 1.0f));
                drawList->AddPolyline(rearWindshield, 4, getFadedColor(IM_COL32(180, 230, 255, 180), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Cabin Roof top
                ImVec2 roofTop[4] = { tCabinRearL, tCabinRearR, tCabinFrontR, tCabinFrontL };
                drawList->AddConvexPolyFilled(roofTop, 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(roofTop, 4, getFadedColor(IM_COL32(255, 255, 255, 120), 1.0f), ImDrawFlags_Closed, 1.5f * z);

                // Side windows on Left Cabin
                ImVec2 wC0 = lerp(tCabinRearL, tCabinFrontL, 0.20f);
                ImVec2 wC1 = lerp(tCabinRearL, tCabinFrontL, 0.80f);
                ImVec2 wCB0 = lerp(mTrunkL, mHoodL, 0.20f);
                ImVec2 wCB1 = lerp(mTrunkL, mHoodL, 0.80f);

                ImVec2 sWinL[4] = {
                    lerp(wCB0, wC0, 0.35f),
                    lerp(wCB1, wC1, 0.35f),
                    wC1,
                    wC0
                };
                drawList->AddConvexPolyFilled(sWinL, 4, getFadedColor(IM_COL32(20, 30, 45, 200), 1.0f));

                // Side windows on Right Cabin
                ImVec2 wCR0 = lerp(tCabinRearR, tCabinFrontR, 0.20f);
                ImVec2 wCR1 = lerp(tCabinRearR, tCabinFrontR, 0.80f);
                ImVec2 wCBR0 = lerp(mTrunkR, mHoodR, 0.20f);
                ImVec2 wCBR1 = lerp(mTrunkR, mHoodR, 0.80f);

                ImVec2 sWinR[4] = {
                    lerp(wCBR0, wCR0, 0.35f),
                    lerp(wCBR1, wCR1, 0.35f),
                    wCR1,
                    wCR0
                };
                drawList->AddConvexPolyFilled(sWinR, 4, getFadedColor(IM_COL32(20, 30, 45, 200), 1.0f));
            }

            // --- 2.5D LIGHTS ---
            float H_front = (vType == 1) ? H * 0.30f : ((vType == 2) ? H * 0.55f : H * 0.52f);
            float H_rear = (vType == 1) ? H * 0.25f : ((vType == 2) ? H * 0.45f : H * 0.58f);

            // Headlights positions (on front face / front hood corners)
            ImVec2 headLightLeft = lerp(p1, p2, 0.20f);
            ImVec2 headLightRight = lerp(p1, p2, 0.80f);
            headLightLeft.y -= (H_front * 0.6f);
            headLightRight.y -= (H_front * 0.6f);

            drawList->AddCircleFilled(headLightLeft, 2.5f * z, getFadedColor(IM_COL32(255, 245, 160, 255), 1.0f));
            drawList->AddCircleFilled(headLightRight, 2.5f * z, getFadedColor(IM_COL32(255, 245, 160, 255), 1.0f));

            // Headlight glow cones at night (pointing forward)
            if (camera.getZoom() > 0.3f) {
                float cdx = p1.x - p0.x;
                float cdy = p1.y - p0.y;
                float clen = std::sqrt(cdx * cdx + cdy * cdy);

                if (clen > 0.1f) {
                    float dirX = cdx / clen;
                    float dirY = cdy / clen;
                    float perpX = -dirY;
                    float perpY = dirX;

                    auto drawGlowCone = [&](ImVec2 origin) {
                        ImVec2 pA = origin;
                        ImVec2 pB(origin.x + dirX * 65.0f * z + perpX * 18.0f * z, origin.y + dirY * 65.0f * z + perpY * 18.0f * z);
                        ImVec2 pC(origin.x + dirX * 65.0f * z - perpX * 18.0f * z, origin.y + dirY * 65.0f * z - perpY * 18.0f * z);

                        drawList->AddTriangleFilled(pA, pB, pC, getFadedColor(IM_COL32(255, 250, 180, 25), 1.0f));
                    };

                    drawGlowCone(headLightLeft);
                    drawGlowCone(headLightRight);
                }
            }

            // Rear lights positions (on rear trunk corners)
            ImVec2 brakeLeft = lerp(p0, p3, 0.20f);
            ImVec2 brakeRight = lerp(p0, p3, 0.80f);
            brakeLeft.y -= (H_rear * 0.6f);
            brakeRight.y -= (H_rear * 0.6f);

            ImU32 brakeCol = vehicle.getIsStopped() ? IM_COL32(255, 10, 10, 255) : IM_COL32(220, 30, 30, 220);

            drawList->AddCircleFilled(brakeLeft, 2.2f * z, getFadedColor(brakeCol, 1.0f));
            drawList->AddCircleFilled(brakeRight, 2.2f * z, getFadedColor(brakeCol, 1.0f));

            // Brake stop flares
            if (vehicle.getIsStopped()) {
                drawList->AddCircleFilled(brakeLeft, 6.0f * z, getFadedColor(IM_COL32(255, 20, 20, 50), 1.0f));
                drawList->AddCircleFilled(brakeRight, 6.0f * z, getFadedColor(IM_COL32(255, 20, 20, 50), 1.0f));
            }
        } else {
            // --- TOP-DOWN 2D VIEW ---
            if (vType == 1) {
                // Bus is a plain box with front windshield and side windows
                std::vector<ImVec2> shadowVertices;
                for (const ImVec2& p : screenVertices) {
                    shadowVertices.push_back(ImVec2(p.x + 3.0f * z, p.y + 4.0f * z));
                }
                drawList->AddConvexPolyFilled(shadowVertices.data(), 4, getFadedColor(IM_COL32(0, 0, 0, 80), 1.0f));

                drawList->AddConvexPolyFilled(screenVertices.data(), 4, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(screenVertices.data(), 4, getFadedColor(IM_COL32(255, 255, 255, 180), 1.0f), ImDrawFlags_Closed, 2.0f * z);

                // Roof/Cabin area
                ImVec2 roof0 = lerp(lerp(p0, p1, 0.05f), lerp(p3, p2, 0.05f), 0.08f);
                ImVec2 roof3 = lerp(lerp(p0, p1, 0.95f), lerp(p3, p2, 0.95f), 0.08f);
                ImVec2 roof2 = lerp(lerp(p3, p2, 0.95f), lerp(p0, p1, 0.95f), 0.08f);
                ImVec2 roof1 = lerp(lerp(p3, p2, 0.05f), lerp(p0, p1, 0.05f), 0.08f);
                ImVec2 roof[4] = { roof0, roof3, roof2, roof1 };
                drawList->AddConvexPolyFilled(roof, 4, getFadedColor(IM_COL32(35, 50, 65, 235), 1.0f));

                // Front Windshield
                ImVec2 w0 = lerp(lerp(p0, p1, 0.88f), lerp(p3, p2, 0.88f), 0.08f);
                ImVec2 w3 = lerp(lerp(p0, p1, 0.94f), lerp(p3, p2, 0.94f), 0.08f);
                ImVec2 w2 = lerp(lerp(p3, p2, 0.94f), lerp(p0, p1, 0.94f), 0.08f);
                ImVec2 w1 = lerp(lerp(p3, p2, 0.88f), lerp(p0, p1, 0.88f), 0.08f);
                ImVec2 wind[4] = { w0, w3, w2, w1 };
                drawList->AddConvexPolyFilled(wind, 4, getFadedColor(IM_COL32(110, 205, 255, 180), 1.0f));

                // Side windows (drawn as multiple small black slots)
                for (int iw = 0; iw < 5; iw++) {
                    float tStart = 0.10f + iw * 0.15f;
                    float tEnd = tStart + 0.10f;

                    // Left Side Windows
                    ImVec2 wL0 = lerp(p0, p1, tStart);
                    ImVec2 wL1 = lerp(p0, p1, tEnd);
                    ImVec2 wL0_in = lerp(wL0, lerp(p3, p2, tStart), 0.07f);
                    ImVec2 wL1_in = lerp(wL1, lerp(p3, p2, tEnd), 0.07f);
                    ImVec2 winL[4] = { wL0, wL1, wL1_in, wL0_in };
                    drawList->AddConvexPolyFilled(winL, 4, getFadedColor(IM_COL32(20, 30, 45, 220), 1.0f));

                    // Right Side Windows
                    ImVec2 wR0 = lerp(p3, p2, tStart);
                    ImVec2 wR1 = lerp(p3, p2, tEnd);
                    ImVec2 wR0_in = lerp(wR0, lerp(p0, p1, tStart), 0.07f);
                    ImVec2 wR1_in = lerp(wR1, lerp(p0, p1, tEnd), 0.07f);
                    ImVec2 winR[4] = { wR0, wR1, wR1_in, wR0_in };
                    drawList->AddConvexPolyFilled(winR, 4, getFadedColor(IM_COL32(20, 30, 45, 220), 1.0f));
                }
            } else {
                // Cars and Trucks have a tapered silhouette
                ImVec2 p0_t1 = lerp(p0, p3, 0.15f);
                ImVec2 p0_t2 = lerp(p0, p1, 0.08f);
                ImVec2 p1_t1 = lerp(p0, p1, 0.88f);
                ImVec2 p1_t2 = lerp(p1, p2, 0.20f);
                ImVec2 p2_t1 = lerp(p1, p2, 0.80f);
                ImVec2 p2_t2 = lerp(p3, p2, 0.88f);
                ImVec2 p3_t1 = lerp(p3, p2, 0.08f);
                ImVec2 p3_t2 = lerp(p0, p3, 0.85f);

                ImVec2 carBody[8] = {
                    p0_t1, p0_t2, p1_t1, p1_t2,
                    p2_t1, p2_t2, p3_t1, p3_t2
                };

                // Ground Shadow
                std::vector<ImVec2> shadowVertices;
                for (int i = 0; i < 8; i++) {
                    shadowVertices.push_back(ImVec2(carBody[i].x + 3.0f * z, carBody[i].y + 4.0f * z));
                }
                drawList->AddConvexPolyFilled(shadowVertices.data(), 8, getFadedColor(IM_COL32(0, 0, 0, 80), 1.0f));

                // Main body
                drawList->AddConvexPolyFilled(carBody, 8, getFadedColor(baseColor, 1.00f));
                drawList->AddPolyline(carBody, 8, getFadedColor(IM_COL32(255, 255, 255, 180), 1.0f), ImDrawFlags_Closed, 2.0f * z);

                // Windows/roof (different for Pickup vs Sedan)
                if (vType == 2) {
                    // Pickup: Cabin from 42% to 76%, Cargo bed from 0% to 42%
                    // Draw Bed Cargo Box
                    ImVec2 bedRL = lerp(lerp(p0, p1, 0.05f), lerp(p3, p2, 0.05f), 0.12f);
                    ImVec2 bedFL = lerp(lerp(p0, p1, 0.40f), lerp(p3, p2, 0.40f), 0.12f);
                    ImVec2 bedFR = lerp(lerp(p3, p2, 0.40f), lerp(p0, p1, 0.40f), 0.12f);
                    ImVec2 bedRR = lerp(lerp(p3, p2, 0.05f), lerp(p0, p1, 0.05f), 0.12f);
                    ImVec2 bed[4] = { bedRL, bedFL, bedFR, bedRR };
                    drawList->AddConvexPolyFilled(bed, 4, getFadedColor(IM_COL32(30, 30, 30, 240), 1.0f));
                    drawList->AddPolyline(bed, 4, getFadedColor(IM_COL32(0, 0, 0, 100), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                    // Cabin Roof
                    ImVec2 roof0 = lerp(lerp(p0, p1, 0.42f), lerp(p3, p2, 0.42f), 0.12f);
                    ImVec2 roof3 = lerp(lerp(p0, p1, 0.76f), lerp(p3, p2, 0.76f), 0.12f);
                    ImVec2 roof2 = lerp(lerp(p3, p2, 0.76f), lerp(p0, p1, 0.76f), 0.12f);
                    ImVec2 roof1 = lerp(lerp(p3, p2, 0.42f), lerp(p0, p1, 0.42f), 0.12f);
                    ImVec2 roof[4] = { roof0, roof3, roof2, roof1 };
                    drawList->AddConvexPolyFilled(roof, 4, getFadedColor(IM_COL32(35, 50, 65, 235), 1.0f));
                    drawList->AddPolyline(roof, 4, getFadedColor(IM_COL32(180, 230, 255, 200), 1.0f), ImDrawFlags_Closed, 1.2f * z);

                    // Front Windshield
                    ImVec2 windL_bot = lerp(lerp(p0, p1, 0.80f), lerp(p3, p2, 0.80f), 0.12f);
                    ImVec2 windR_bot = lerp(lerp(p3, p2, 0.80f), lerp(p0, p1, 0.80f), 0.12f);
                    ImVec2 windshield[4] = { windL_bot, roof3, roof2, windR_bot };
                    drawList->AddConvexPolyFilled(windshield, 4, getFadedColor(IM_COL32(120, 210, 255, 160), 1.0f));
                } else {
                    // Sedan: Cabin from 24% to 72%
                    // Cabin Roof
                    ImVec2 roof0 = lerp(lerp(p0, p1, 0.24f), lerp(p3, p2, 0.24f), 0.12f);
                    ImVec2 roof3 = lerp(lerp(p0, p1, 0.72f), lerp(p3, p2, 0.72f), 0.12f);
                    ImVec2 roof2 = lerp(lerp(p3, p2, 0.72f), lerp(p0, p1, 0.72f), 0.12f);
                    ImVec2 roof1 = lerp(lerp(p3, p2, 0.24f), lerp(p0, p1, 0.24f), 0.12f);
                    ImVec2 roof[4] = { roof0, roof3, roof2, roof1 };
                    drawList->AddConvexPolyFilled(roof, 4, getFadedColor(IM_COL32(35, 50, 65, 235), 1.0f));
                    drawList->AddPolyline(roof, 4, getFadedColor(IM_COL32(180, 230, 255, 200), 1.0f), ImDrawFlags_Closed, 1.2f * z);

                    // Front Windshield
                    ImVec2 windFrontL_bot = lerp(lerp(p0, p1, 0.76f), lerp(p3, p2, 0.76f), 0.12f);
                    ImVec2 windFrontR_bot = lerp(lerp(p3, p2, 0.76f), lerp(p0, p1, 0.76f), 0.12f);
                    ImVec2 windshield[4] = { windFrontL_bot, roof3, roof2, windFrontR_bot };
                    drawList->AddConvexPolyFilled(windshield, 4, getFadedColor(IM_COL32(120, 210, 255, 160), 1.0f));

                    // Rear Windshield
                    ImVec2 windRearL_bot = lerp(lerp(p0, p1, 0.20f), lerp(p3, p2, 0.20f), 0.12f);
                    ImVec2 windRearR_bot = lerp(lerp(p3, p2, 0.20f), lerp(p0, p1, 0.20f), 0.12f);
                    ImVec2 rearWindshield[4] = { windRearL_bot, roof0, roof1, windRearR_bot };
                    drawList->AddConvexPolyFilled(rearWindshield, 4, getFadedColor(IM_COL32(120, 210, 255, 160), 1.0f));
                }
            }

            // Wheels (with hubs)
            ImVec2 wA = lerp(p0, p1, 0.22f); // Rear-left wheel
            ImVec2 wB = lerp(p0, p1, 0.78f); // Front-left wheel
            ImVec2 wC = lerp(p3, p2, 0.22f); // Rear-right wheel
            ImVec2 wD = lerp(p3, p2, 0.78f); // Front-right wheel

            float wRad = 3.8f * z;
            drawList->AddCircleFilled(wA, wRad, getFadedColor(IM_COL32(10, 10, 10, 255), 1.0f));
            drawList->AddCircleFilled(wB, wRad, getFadedColor(IM_COL32(10, 10, 10, 255), 1.0f));
            drawList->AddCircleFilled(wC, wRad, getFadedColor(IM_COL32(10, 10, 10, 255), 1.0f));
            drawList->AddCircleFilled(wD, wRad, getFadedColor(IM_COL32(10, 10, 10, 255), 1.0f));

            drawList->AddCircleFilled(wA, wRad * 0.45f, getFadedColor(IM_COL32(160, 160, 160, 255), 1.0f));
            drawList->AddCircleFilled(wB, wRad * 0.45f, getFadedColor(IM_COL32(160, 160, 160, 255), 1.0f));
            drawList->AddCircleFilled(wC, wRad * 0.45f, getFadedColor(IM_COL32(160, 160, 160, 255), 1.0f));
            drawList->AddCircleFilled(wD, wRad * 0.45f, getFadedColor(IM_COL32(160, 160, 160, 255), 1.0f));

            // Headlights
            ImVec2 hL1 = lerp(p1, p2, 0.25f);
            ImVec2 hL2 = lerp(p1, p2, 0.75f);
            drawList->AddCircleFilled(hL1, 2.8f * z, getFadedColor(IM_COL32(255, 245, 150, 255), 1.0f));
            drawList->AddCircleFilled(hL2, 2.8f * z, getFadedColor(IM_COL32(255, 245, 150, 255), 1.0f));

            // Rear Lights
            ImVec2 rL1 = lerp(p0, p3, 0.25f);
            ImVec2 rL2 = lerp(p0, p3, 0.75f);
            ImU32 brakeCol = vehicle.getIsStopped() ? IM_COL32(255, 10, 10, 255) : IM_COL32(220, 30, 30, 220);
            drawList->AddCircleFilled(rL1, 2.4f * z, getFadedColor(brakeCol, 1.0f));
            drawList->AddCircleFilled(rL2, 2.4f * z, getFadedColor(brakeCol, 1.0f));

            if (vehicle.getIsStopped()) {
                drawList->AddCircleFilled(rL1, 6.0f * z, getFadedColor(IM_COL32(255, 20, 20, 50), 1.0f));
                drawList->AddCircleFilled(rL2, 6.0f * z, getFadedColor(IM_COL32(255, 20, 20, 50), 1.0f));
            }
        }

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
    ImVec2 panelSize(380.0f, 235.0f);

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
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 82), IM_COL32(230, 230, 230, 255), "Fill: Scan-line Polygon Fill (Odd-Even Rule)");
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 102), IM_COL32(230, 230, 230, 255), "Clip: Cohen-Sutherland Line Clipping");
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 122), IM_COL32(230, 230, 230, 255), viewMode);
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 142), IM_COL32(230, 230, 230, 255), "Transform: Translation x Rotation x Scaling");

    char buffer[128];

    std::snprintf(buffer, sizeof(buffer), "Roads: %d  Buildings: %d", static_cast<int>(area.roads.size()), static_cast<int>(area.buildings.size()));
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 168), IM_COL32(180, 220, 255, 255), buffer);

    std::snprintf(buffer, sizeof(buffer), "Vehicles: %d  Signals: %d", static_cast<int>(vehicles.size()), static_cast<int>(trafficLights.size()));
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 188), IM_COL32(180, 220, 255, 255), buffer);

    std::snprintf(buffer, sizeof(buffer), "Camera Zoom: %.2f", camera.getZoom());
    drawList->AddText(ImVec2(panelPos.x + 12, panelPos.y + 208), IM_COL32(180, 220, 255, 255), buffer);

    // Draw the Cohen-Sutherland clip window outline on screen
    ImVec2 dispSize = ImGui::GetIO().DisplaySize;
    float cm = 80.0f;
    drawList->AddRect(
        ImVec2(cm, cm),
        ImVec2(dispSize.x - cm, dispSize.y - cm),
        IM_COL32(255, 80, 80, 220),
        0.0f, 0, 2.5f
    );
    drawList->AddText(
        ImVec2(cm + 6.0f, cm + 4.0f),
        IM_COL32(255, 80, 80, 255),
        "Cohen-Sutherland Clip Window"
    );
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

void Renderer::drawGround(const LiveContextEngine& liveContext) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImU32 groundColor;
    if (liveContext.isNightMode()) {
        groundColor = IM_COL32(14, 18, 26, 255);
    } else if (liveContext.isRainMode()) {
        groundColor = IM_COL32(32, 38, 40, 255);
    } else {
        groundColor = IM_COL32(38, 48, 40, 255); // Stylish dark forest green
    }

    // Fill the ground
    drawList->AddRectFilled(ImVec2(0, 0), displaySize, groundColor);

    // Draw subtle grid lines
    float gridSize = 60.0f;
    ImU32 gridColor = liveContext.isNightMode() ? IM_COL32(255, 255, 255, 6) : IM_COL32(255, 255, 255, 12);
    for (float x = 0; x < displaySize.x; x += gridSize) {
        drawList->AddLine(ImVec2(x, 0), ImVec2(x, displaySize.y), gridColor, 1.0f);
    }
    for (float y = 0; y < displaySize.y; y += gridSize) {
        drawList->AddLine(ImVec2(0, y), ImVec2(displaySize.x, y), gridColor, 1.0f);
    }
}

void Renderer::drawEnvironmentDetails(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

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
    const int maxTrees = 150; // Increased to accommodate the larger Pettah map

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

                // Tree shadow on ground
                drawList->AddCircleFilled(
                    ImVec2(screen.x + 4.0f * z, screen.y + 6.0f * z),
                    8.0f * z,
                    IM_COL32(0, 0, 0, 70)
                );

                // Select tree variety based on position pseudo-random seed
                int tVariety = (s + treeCount) % 3;

                if (tVariety == 0) {
                    // Variety 0: Classic Green Oak Tree with Fruit
                    // Trunk
                    drawList->AddRectFilled(
                        ImVec2(screen.x - 2.0f * z, screen.y + 4.0f * z),
                        ImVec2(screen.x + 2.0f * z, screen.y + 13.0f * z),
                        IM_COL32(110, 75, 35, 240)
                    );

                    // Layered leaves
                    drawList->AddCircleFilled(
                        ImVec2(screen.x, screen.y + 2.0f * z),
                        10.0f * z,
                        IM_COL32(20, 110, 50, 240) // Shadow layer
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x, screen.y),
                        9.0f * z,
                        IM_COL32(35, 150, 75, 240) // Main canopy
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x - 5.0f * z, screen.y - 3.0f * z),
                        6.5f * z,
                        IM_COL32(45, 175, 85, 230) // Left highlight
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x + 5.0f * z, screen.y + 2.0f * z),
                        6.5f * z,
                        IM_COL32(25, 125, 60, 230) // Right shade
                    );

                    // Red apples/blossom dots
                    drawList->AddCircleFilled(ImVec2(screen.x - 3.0f * z, screen.y + 1.0f * z), 1.2f * z, IM_COL32(235, 60, 60, 240));
                    drawList->AddCircleFilled(ImVec2(screen.x + 2.0f * z, screen.y - 2.0f * z), 1.2f * z, IM_COL32(235, 60, 60, 240));
                    drawList->AddCircleFilled(ImVec2(screen.x - 1.0f * z, screen.y - 4.0f * z), 1.2f * z, IM_COL32(235, 60, 60, 240));

                } else if (tVariety == 1) {
                    // Variety 1: Coniferous Pine Tree (Layered Triangles)
                    // Trunk
                    drawList->AddRectFilled(
                        ImVec2(screen.x - 1.8f * z, screen.y + 4.0f * z),
                        ImVec2(screen.x + 1.8f * z, screen.y + 14.0f * z),
                        IM_COL32(90, 60, 30, 240)
                    );

                    // Pine Leaf Layers (Bottom to Top)
                    // Bottom triangle
                    ImVec2 pA_bot(screen.x, screen.y - 6.0f * z);
                    ImVec2 pB_bot(screen.x - 9.0f * z, screen.y + 5.0f * z);
                    ImVec2 pC_bot(screen.x + 9.0f * z, screen.y + 5.0f * z);
                    drawList->AddTriangleFilled(pA_bot, pB_bot, pC_bot, IM_COL32(20, 95, 55, 240));

                    // Middle triangle
                    ImVec2 pA_mid(screen.x, screen.y - 12.0f * z);
                    ImVec2 pB_mid(screen.x - 7.5f * z, screen.y - 1.0f * z);
                    ImVec2 pC_mid(screen.x + 7.5f * z, screen.y - 1.0f * z);
                    drawList->AddTriangleFilled(pA_mid, pB_mid, pC_mid, IM_COL32(25, 115, 65, 240));

                    // Top triangle
                    ImVec2 pA_top(screen.x, screen.y - 18.0f * z);
                    ImVec2 pB_top(screen.x - 5.5f * z, screen.y - 7.0f * z);
                    ImVec2 pC_top(screen.x + 5.5f * z, screen.y - 7.0f * z);
                    drawList->AddTriangleFilled(pA_top, pB_top, pC_top, IM_COL32(35, 140, 75, 240));

                } else {
                    // Variety 2: Cherry Blossom Tree (Pink Canopy with White Accents)
                    // Trunk
                    drawList->AddRectFilled(
                        ImVec2(screen.x - 2.0f * z, screen.y + 4.0f * z),
                        ImVec2(screen.x + 2.0f * z, screen.y + 13.0f * z),
                        IM_COL32(110, 80, 50, 240)
                    );

                    // Pink Canopy Layers
                    drawList->AddCircleFilled(
                        ImVec2(screen.x, screen.y + 2.0f * z),
                        10.0f * z,
                        IM_COL32(200, 70, 110, 240) // Dark pink shade
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x, screen.y),
                        9.0f * z,
                        IM_COL32(240, 110, 160, 240) // Main pink
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x - 5.0f * z, screen.y - 3.0f * z),
                        6.5f * z,
                        IM_COL32(255, 150, 190, 230) // Light pink highlight
                    );
                    drawList->AddCircleFilled(
                        ImVec2(screen.x + 5.0f * z, screen.y + 2.0f * z),
                        6.5f * z,
                        IM_COL32(220, 90, 130, 230) // Medium pink
                    );

                    // White flower accents
                    drawList->AddCircleFilled(ImVec2(screen.x - 2.0f * z, screen.y - 2.0f * z), 1.2f * z, IM_COL32(255, 255, 255, 240));
                    drawList->AddCircleFilled(ImVec2(screen.x + 3.0f * z, screen.y + 1.0f * z), 1.2f * z, IM_COL32(255, 255, 255, 240));
                    drawList->AddCircleFilled(ImVec2(screen.x - 1.0f * z, screen.y + 4.0f * z), 1.2f * z, IM_COL32(255, 255, 255, 240));
                }

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
    // Windows are now drawn directly on 2.5D building side face walls with perspective mapping.
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

void Renderer::drawPedestrianCrossings(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    float z = camera.getZoom();

    for (const PedestrianCrossing& crossing : area.crossings) {
        if (crossing.points.size() < 2) continue;

        for (size_t i = 0; i + 1 < crossing.points.size(); i++) {
            Vec2 a = transformForView(crossing.points[i], isometricMode);
            Vec2 b = transformForView(crossing.points[i + 1], isometricMode);

            a = applyCamera(a, camera);
            b = applyCamera(b, camera);

            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len <= 0.1f) continue;

            float nx = -dy / len;
            float ny = dx / len;

            float stepSize = 8.0f * z;
            float stripeHalfLen = 9.0f * z;

            float dist = 0.0f;
            while (dist < len) {
                float t = dist / len;
                ImVec2 center(a.x + dx * t, a.y + dy * t);

                ImVec2 start(center.x - nx * stripeHalfLen, center.y - ny * stripeHalfLen);
                ImVec2 end(center.x + nx * stripeHalfLen, center.y + ny * stripeHalfLen);

                drawList->AddLine(
                    start,
                    end,
                    IM_COL32(245, 245, 245, 190),
                    4.0f * z
                );

                dist += stepSize;
            }
        }
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