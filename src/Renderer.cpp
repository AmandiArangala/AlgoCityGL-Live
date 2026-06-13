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
#include <algorithm>

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
    drawGround(area, isometricMode, camera, liveContext);

    // 2. Draw roads and ground-level elements
    drawRoads(area, isometricMode, camera);
    drawEnvironmentDetails(area, isometricMode, camera);
    drawStopLines(trafficLights, isometricMode, camera);
    drawPedestrianCrossings(area, isometricMode, camera);
    
    // Draw nature and entities BEFORE buildings so they are occluded properly
    drawTrees(area, isometricMode, camera);
    drawPedestriansAndPets(area, isometricMode, camera);
    drawRuntimeTrafficLights(trafficLights, isometricMode, camera);
    drawVehicles(vehicles, isometricMode, camera, liveContext);

    // 3. Draw Buildings (tall structures that must occlude things behind them)
    if (isometricMode) {
        drawBuildingFills2_5D(area, liveContext, camera);
    } else {
        drawTopDownBuildingFills(area, liveContext, camera);
    }
    drawBuildingWindows(area, isometricMode, camera);

    // 4. Raster pixel scene
    buildCityPixelScene(area, selectedLineAlgorithm, xrayMode, isometricMode, camera);
    drawPixelBuffer(xrayMode);

    // 5. Draw weather/night tint overlay over the whole scene
    drawLiveContextOverlay(liveContext);

    if (liveContext.isNightMode()) {
        drawNightEffect(area, isometricMode, camera);
    }

    if (liveContext.isIncidentMode()) {
        drawIncidentMarker(isometricMode, camera);
    }

    if (liveContext.isRainMode()) {
        drawRainEffect();
    }

    // 6. Draw UI elements and Labels on top of everything
    drawMiniMap(area, vehicles, camera);
    drawRoadLabels(area, isometricMode, camera);
    drawBuildingLabels(area, isometricMode, camera);

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
        staged.roadWidth = (road.lanes == 1 ? 40.0f : (road.lanes == 2 ? 65.0f : (road.lanes == 6 ? 115.0f : 90.0f))) * z; // Wider roads
        staged.sidewalkWidth = staged.roadWidth + 18.0f * z;
        staged.curbWidth = staged.roadWidth + 4.0f * z;

        staged.points.reserve(road.points.size());
        for (const Vec2& p : road.points) {
            Vec2 proj = transformForView(p, isometricMode);
            proj = applyCamera(proj, camera);
            staged.points.push_back(ImVec2(proj.x, proj.y));
        }
        stagedRoads.push_back(staged);
    }

    // Pass 1: Draw Sidewalks (Tan Base)
    ImU32 sidewalkColor = IM_COL32(180, 160, 140, 255);
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
    ImU32 asphaltColor = IM_COL32(60, 60, 65, 255);
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

    // Base setup for dashed lines
    float dashLen = 14.0f * z;
    float gapLen = 10.0f * z;
    float totalStep = dashLen + gapLen;

    auto drawDashedLine = [&](float sideOffset) {
        float dist = 0.0f;
        while (dist < len) {
            float startT = dist / len;
            float endT = std::min(dist + dashLen, len) / len;

            ImVec2 pt1(a.x + dx * startT + nx * sideOffset, a.y + dy * startT + ny * sideOffset);
            ImVec2 pt2(a.x + dx * endT + nx * sideOffset, a.y + dy * endT + ny * sideOffset);

            drawList->AddLine(pt1, pt2, IM_COL32(255, 255, 255, 220), 2.5f * z);
            dist += totalStep;
        }
    };

    auto drawSolidLine = [&](float sideOffset) {
        ImVec2 pt1(a.x + nx * sideOffset, a.y + ny * sideOffset);
        ImVec2 pt2(b.x + nx * sideOffset, b.y + ny * sideOffset);
        drawList->AddLine(pt1, pt2, IM_COL32(255, 255, 255, 255), 3.0f * z);
    };

    if (lanes == 1) {
        drawDashedLine(0.0f);
    } else if (lanes == 2) {
        drawDashedLine(0.0f);
    } else if (lanes == 6) {
        // 3 lanes each direction with a solid center divider
        drawSolidLine(0.0f);
        drawDashedLine(roadWidth * (1.0f / 6.0f));
        drawDashedLine(roadWidth * (2.0f / 6.0f));
        drawDashedLine(-roadWidth * (1.0f / 6.0f));
        drawDashedLine(-roadWidth * (2.0f / 6.0f));
    } else {
        // 3+ lanes: center dashed and side dashed
        drawDashedLine(0.0f);
        drawDashedLine(roadWidth * 0.25f);
        drawDashedLine(-roadWidth * 0.25f);

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
                worldCenter.x + (point.x - worldCenter.x) * 0.55f,
                worldCenter.y + (point.y - worldCenter.y) * 0.55f
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

        // Compute bounding box minimums for robust isometric depth sorting
        float minX = 999999.0f;
        float minY = 999999.0f;
        for (const Vec2& pt : b.base) {
            if (pt.x < minX) minX = pt.x;
            if (pt.y < minY) minY = pt.y;
        }

        // Depth metric: back-to-front sorting.
        // Buildings that start further back (smaller minX + minY) are drawn first.
        float depth = minX + minY;
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
            // Shrink footprint towards center (scale factor 0.55)
            Vec2 shrunkPoint(
                center.x + (point.x - center.x) * 0.55f,
                center.y + (point.y - center.y) * 0.55f
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

        // Determine winding of the base polygon in screen space
        float baseArea = 0.0f;
        for (size_t i = 0; i < base.size(); i++) {
            size_t next = (i + 1) % base.size();
            baseArea += (base[next].x - base[i].x) * (base[next].y + base[i].y);
        }
        bool isClockwise = (baseArea < 0.0f);

        // 2. Draw Side Faces (Walls) with Perspective Shading & Textures
        for (size_t i = 0; i < base.size(); i++) {
            size_t next = (i + 1) % base.size();

            float dx = base[next].x - base[i].x;
            bool isFrontFace = isClockwise ? (dx <= 0.0f) : (dx >= 0.0f);
            if (!isFrontFace) continue;

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
                // Determine windows count based on building height
                int num_rows = 3 + static_cast<int>(building.height / 30.0f);
                bool isNight = liveContext.isNightMode();

                for (int r = 0; r < num_rows; r++) {
                    float tx0 = 0.04f;
                    float tx1 = 0.96f;
                    float ty0 = (r + 0.30f) / num_rows;
                    float ty1 = (r + 0.70f) / num_rows;

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

                    // Dark continuous window strip (unless night lit)
                    ImU32 windowColor;
                    if (isNight) {
                        int hash = (r * 13 + buildingIndex * 97 + static_cast<int>(i) * 11) % 100;
                        if (hash < 25) {
                            windowColor = IM_COL32(255, 215, 80, 220); // Warm Lit
                        } else {
                            windowColor = IM_COL32(22, 26, 35, 240); // Dark
                        }
                    } else {
                        windowColor = IM_COL32(25, 30, 35, 245); // Dark tinted window strip
                    }

                    drawList->AddConvexPolyFilled(windowPane, 4, windowColor);
                    drawList->AddPolyline(windowPane, 4, IM_COL32(10, 15, 20, 100), ImDrawFlags_Closed, 0.5f * z);
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

        // 5. Draw Billboards (on select rooftops)
        if (buildingIndex % 5 == 0 && top.size() >= 2) {
            ImVec2 p0 = top[0];
            ImVec2 p1 = top[1];
            
            ImVec2 poleTop0(p0.x, p0.y - 12.0f * z);
            ImVec2 poleTop1(p1.x, p1.y - 12.0f * z);
            
            // Poles
            drawList->AddLine(p0, poleTop0, IM_COL32(120, 120, 120, 255), 2.5f * z);
            drawList->AddLine(p1, poleTop1, IM_COL32(120, 120, 120, 255), 2.5f * z);
            
            // Billboard Board
            float bbHeight = 18.0f * z;
            ImVec2 bb0 = poleTop0;
            ImVec2 bb1 = poleTop1;
            ImVec2 bb2(bb1.x, bb1.y - bbHeight);
            ImVec2 bb3(bb0.x, bb0.y - bbHeight);
            
            ImVec2 bbFace[4] = { bb0, bb1, bb2, bb3 };
            drawList->AddConvexPolyFilled(bbFace, 4, IM_COL32(245, 245, 250, 255)); // Blank White Billboard
            drawList->AddPolyline(bbFace, 4, IM_COL32(50, 50, 50, 255), ImDrawFlags_Closed, 1.5f * z); // Grey Frame
        }

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
                    center.x + (ptA.x - center.x) * 0.55f,
                    center.y + (ptA.y - center.y) * 0.55f
                );
                Vec2 shrunkB(
                    center.x + (ptB.x - center.x) * 0.55f,
                    center.y + (ptB.y - center.y) * 0.55f
                );

                Vec2 a = applyCamera(shrunkA, camera);
                Vec2 b = applyCamera(shrunkB, camera);

                drawLineClipped(a, b, buildingColor, false);
            }
        } else {
            std::vector<Vec2> base;
            std::vector<Vec2> top;

            for (const Vec2& point : building.base) {
                // Shrink footprint towards center (scale factor 0.55)
                Vec2 shrunkPoint(
                    center.x + (point.x - center.x) * 0.55f,
                    center.y + (point.y - center.y) * 0.55f
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
    const Camera2D& camera,
    const LiveContextEngine& liveContext
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
        int vType = vehicle.getType();
        float hVal = 8.0f; // Default height
        ImU32 baseColor;

        switch (vType) {
            case Vehicle::CAR: // Sedan/Standard car (Orange)
                hVal = 9.0f;
                baseColor = IM_COL32(235, 110, 30, 245);
                break;
            case Vehicle::BUS: // Bus (Blue)
                hVal = 17.0f; // Tall
                baseColor = IM_COL32(40, 130, 240, 245);
                break;
            case Vehicle::TRUCK: // SUV/Truck (White/Silver)
                hVal = 15.0f;
                baseColor = IM_COL32(220, 220, 225, 245);
                break;
            case Vehicle::BIKE: // Bike (Yellow/Greenish)
                hVal = 6.0f; // Low profile
                baseColor = IM_COL32(40, 230, 70, 245);
                break;
            default:
                hVal = 9.0f;
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
                // --- BUS SHAPE (Dual-tone White Top, Blue Bottom) ---
                ImVec2 t0(p0.x, p0.y - H * 0.6f);
                ImVec2 t1(p1.x, p1.y - H * 0.6f);
                ImVec2 t2(p2.x, p2.y - H * 0.6f);
                ImVec2 t3(p3.x, p3.y - H * 0.6f);

                ImVec2 m0 = lerp(p0, t0, 0.4f); // Mid points for dual tone
                ImVec2 m1 = lerp(p1, t1, 0.4f);
                ImVec2 m2 = lerp(p2, t2, 0.4f);
                ImVec2 m3 = lerp(p3, t3, 0.4f);

                ImU32 busWhite = IM_COL32(235, 240, 245, 245);
                ImU32 busBlue = IM_COL32(35, 105, 160, 245); // Darker blue matching the image

                // Rear Face
                ImVec2 rearFace[4] = { p0, p3, t3, t0 };
                drawList->AddConvexPolyFilled(rearFace, 4, getFadedColor(busWhite, 0.70f));
                ImVec2 rearFaceBot[4] = { p0, p3, m3, m0 };
                drawList->AddConvexPolyFilled(rearFaceBot, 4, getFadedColor(busBlue, 0.70f));
                drawList->AddPolyline(rearFace, 4, getFadedColor(IM_COL32(0, 0, 0, 40), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Left Face (Side)
                ImVec2 leftFace[4] = { p0, p1, t1, t0 };
                drawList->AddConvexPolyFilled(leftFace, 4, getFadedColor(busWhite, 1.15f));
                ImVec2 leftFaceBot[4] = { p0, p1, m1, m0 };
                drawList->AddConvexPolyFilled(leftFaceBot, 4, getFadedColor(busBlue, 1.15f));
                drawList->AddPolyline(leftFace, 4, getFadedColor(IM_COL32(255, 255, 255, 60), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Right Face (Side)
                ImVec2 rightFace[4] = { p3, p2, t2, t3 };
                drawList->AddConvexPolyFilled(rightFace, 4, getFadedColor(busWhite, 0.85f));
                ImVec2 rightFaceBot[4] = { p3, p2, m2, m3 };
                drawList->AddConvexPolyFilled(rightFaceBot, 4, getFadedColor(busBlue, 0.85f));
                drawList->AddPolyline(rightFace, 4, getFadedColor(IM_COL32(0, 0, 0, 50), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Front Face
                ImVec2 frontFace[4] = { p1, p2, t2, t1 };
                drawList->AddConvexPolyFilled(frontFace, 4, getFadedColor(busWhite, 1.05f));
                ImVec2 frontFaceBot[4] = { p1, p2, m2, m1 };
                drawList->AddConvexPolyFilled(frontFaceBot, 4, getFadedColor(busBlue, 1.05f));
                drawList->AddPolyline(frontFace, 4, getFadedColor(IM_COL32(255, 255, 255, 80), 1.0f), ImDrawFlags_Closed, 1.0f * z);

                // Top Roof Face
                ImVec2 topFace[4] = { t0, t1, t2, t3 };
                drawList->AddConvexPolyFilled(topFace, 4, getFadedColor(busWhite, 1.00f));
                drawList->AddPolyline(topFace, 4, getFadedColor(IM_COL32(255, 255, 255, 120), 1.0f), ImDrawFlags_Closed, 1.5f * z);

                // Bus side passenger windows (Continuous dark strip)
                // Bus side passenger windows (Continuous dark strip)
                float tStart = 0.05f;
                float tEnd = 0.95f;

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
                drawList->AddConvexPolyFilled(winL, 4, getFadedColor(IM_COL32(20, 30, 45, 240), 1.0f));

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
                drawList->AddConvexPolyFilled(winR, 4, getFadedColor(IM_COL32(20, 30, 45, 240), 1.0f));
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
            if (!liveContext.isSunnyMode()) {
                ImVec2 headLightLeft = lerp(p1, p2, 0.20f);
                ImVec2 headLightRight = lerp(p1, p2, 0.80f);
                headLightLeft.y -= (H_front * 0.6f);
                headLightRight.y -= (H_front * 0.6f);

                // Increased headlight size
                drawList->AddCircleFilled(headLightLeft, 3.5f * z, getFadedColor(IM_COL32(255, 245, 160, 255), 1.0f));
                drawList->AddCircleFilled(headLightRight, 3.5f * z, getFadedColor(IM_COL32(255, 245, 160, 255), 1.0f));

                // Increased Headlight glow cones at night/rain (pointing forward)
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
                            ImVec2 pB(origin.x + dirX * 100.0f * z + perpX * 30.0f * z, origin.y + dirY * 100.0f * z + perpY * 30.0f * z);
                            ImVec2 pC(origin.x + dirX * 100.0f * z - perpX * 30.0f * z, origin.y + dirY * 100.0f * z - perpY * 30.0f * z);

                            drawList->AddTriangleFilled(pA, pB, pC, getFadedColor(IM_COL32(255, 250, 180, 45), 1.0f));
                        };

                        drawGlowCone(headLightLeft);
                        drawGlowCone(headLightRight);
                    }
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
            if (!liveContext.isSunnyMode()) {
                ImVec2 hL1 = lerp(p1, p2, 0.25f);
                ImVec2 hL2 = lerp(p1, p2, 0.75f);
                drawList->AddCircleFilled(hL1, 4.5f * z, getFadedColor(IM_COL32(255, 245, 150, 255), 1.0f));
                drawList->AddCircleFilled(hL2, 4.5f * z, getFadedColor(IM_COL32(255, 245, 150, 255), 1.0f));
                
                // Top-down glow cone
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
                        ImVec2 pB(origin.x + dirX * 100.0f * z + perpX * 30.0f * z, origin.y + dirY * 100.0f * z + perpY * 30.0f * z);
                        ImVec2 pC(origin.x + dirX * 100.0f * z - perpX * 30.0f * z, origin.y + dirY * 100.0f * z - perpY * 30.0f * z);
                        drawList->AddTriangleFilled(pA, pB, pC, getFadedColor(IM_COL32(255, 250, 180, 45), 1.0f));
                    };
                    drawGlowCone(hL1);
                    drawGlowCone(hL2);
                }
            }

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
    float time = static_cast<float>(ImGui::GetTime());

    // Wind shear/slant
    float slant = 15.0f;
    float dropSpeed = 800.0f;

    for (int i = 0; i < 250; i++) {
        // Random deterministic seed
        float seed = static_cast<float>(i * 3821);
        
        // Initial drop position calculation
        float start_x = fmod(seed, displaySize.x + 200.0f);
        float y = fmod(seed * 1.3f + time * (dropSpeed + (i % 50) * 10.0f), displaySize.y);
        
        // Apply slant to X
        float x = start_x - (y / displaySize.y) * (slant * 10.0f);
        
        // Wrap X just in case it goes off screen left
        if (x < -20.0f) {
            x += displaySize.x + 40.0f;
        }

        // Drop length and color variation
        float length = 20.0f + (i % 10);
        int alpha = 100 + (i % 100);
        
        drawList->AddLine(
            ImVec2(x, y),
            ImVec2(x - slant, y + length),
            IM_COL32(180, 210, 255, alpha),
            1.5f
        );

        // Ground splash effect (if it's hitting the bottom third of the screen)
        if (y > displaySize.y * 0.8f && (i % 3) == 0) {
            float splashAlphaMod = 1.0f - (y - displaySize.y * 0.8f) / (displaySize.y * 0.2f);
            int splashAlpha = static_cast<int>(80 * splashAlphaMod);
            drawList->AddCircle(
                ImVec2(x - slant, y + length),
                3.0f + fmod(time * 15.0f + i, 5.0f), // Expanding ripple
                IM_COL32(200, 230, 255, splashAlpha),
                0,
                1.0f
            );
        }
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

        // Move label to the top of the roof if in isometric mode
        if (isometricMode) {
            pos.y -= (building.height * 0.6f * camera.getZoom());
        }

        float poleHeight = 25.0f * camera.getZoom();

        // Draw a pole from the roof to the board
        if (isometricMode) {
            drawList->AddLine(
                ImVec2(pos.x, pos.y),
                ImVec2(pos.x, pos.y - poleHeight),
                IM_COL32(180, 180, 180, 220),
                3.0f * camera.getZoom()
            );
        }

        pos.y -= poleHeight;

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

void Renderer::drawGround(const CityArea& area, bool isometricMode, const Camera2D& camera, const LiveContextEngine& liveContext) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // 1. Mainland Fill (Entire Screen)
    ImU32 groundColor;
    if (liveContext.isNightMode()) {
        groundColor = IM_COL32(20, 26, 30, 255);
    } else if (liveContext.isRainMode()) {
        groundColor = IM_COL32(32, 38, 40, 255);
    } else if (liveContext.isSunnyMode()) {
        groundColor = IM_COL32(110, 155, 90, 255); // Vibrant bright green/brown
    } else {
        groundColor = IM_COL32(38, 48, 40, 255); // Stylish dark forest green
    }

    drawList->AddRectFilled(ImVec2(0, 0), displaySize, groundColor);

    // 2. Grid Lines (Entire Screen)
    float gridSize = 60.0f * camera.getZoom();
    ImU32 gridColor = liveContext.isNightMode() ? IM_COL32(255, 255, 255, 6) : IM_COL32(255, 255, 255, 12);

    float offsetX = fmod(camera.getOffsetX(), gridSize);
    float offsetY = fmod(camera.getOffsetY(), gridSize);
    
    if (offsetX > 0) offsetX -= gridSize;
    if (offsetY > 0) offsetY -= gridSize;

    for (float x = offsetX; x < displaySize.x; x += gridSize) {
        drawList->AddLine(ImVec2(x, 0), ImVec2(x, displaySize.y), gridColor);
    }
    for (float y = offsetY; y < displaySize.y; y += gridSize) {
        drawList->AddLine(ImVec2(0, y), ImVec2(displaySize.x, y), gridColor);
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

        float z = camera.getZoom();

        drawList->AddCircleFilled(
            ImVec2(p.x, p.y),
            22.0f * z,
            IM_COL32(60, 60, 65, 130)
        );

        drawList->AddCircle(
            ImVec2(p.x, p.y),
            22.0f * z,
            IM_COL32(210, 210, 210, 130),
            24,
            1.5f * z
        );
    }

    // Draw Monumental Statue at the first major intersection
    if (!area.trafficLights.empty() && camera.getZoom() > 0.3f) {
        Vec2 monumentPos = area.trafficLights[0].position;
        Vec2 screen = transformForView(monumentPos, isometricMode);
        screen = applyCamera(screen, camera);
        
        float z = camera.getZoom();
        
        // Tier 1 circular base
        drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 45.0f * z, IM_COL32(130, 135, 140, 255));
        drawList->AddCircle(ImVec2(screen.x, screen.y), 45.0f * z, IM_COL32(100, 105, 110, 255), 24, 2.0f * z);
        
        // Tier 2 circular base
        ImVec2 t2Pos(screen.x, screen.y - 10.0f * z);
        drawList->AddCircleFilled(t2Pos, 35.0f * z, IM_COL32(150, 155, 160, 255));
        drawList->AddCircle(t2Pos, 35.0f * z, IM_COL32(120, 125, 130, 255), 24, 1.5f * z);
        
        // Tall square pillar
        ImVec2 pillarBaseL(screen.x - 12.0f * z, t2Pos.y - 5.0f * z);
        ImVec2 pillarBaseR(screen.x + 12.0f * z, t2Pos.y - 5.0f * z);
        ImVec2 pillarTopL(screen.x - 10.0f * z, t2Pos.y - 55.0f * z);
        ImVec2 pillarTopR(screen.x + 10.0f * z, t2Pos.y - 55.0f * z);
        
        ImVec2 pillarFace[4] = { pillarBaseL, pillarBaseR, pillarTopR, pillarTopL };
        drawList->AddConvexPolyFilled(pillarFace, 4, IM_COL32(170, 175, 180, 255));
        drawList->AddPolyline(pillarFace, 4, IM_COL32(120, 120, 120, 255), ImDrawFlags_Closed, 1.5f * z);
        
        // Statue figure (geometric approximation: body, head, arms)
        ImVec2 bodyBase(screen.x, t2Pos.y - 55.0f * z);
        ImVec2 bodyTop(screen.x, t2Pos.y - 85.0f * z);
        drawList->AddLine(bodyBase, bodyTop, IM_COL32(90, 95, 100, 255), 8.0f * z); // Body
        
        ImVec2 head(screen.x, t2Pos.y - 92.0f * z);
        drawList->AddCircleFilled(head, 5.0f * z, IM_COL32(90, 95, 100, 255)); // Head
        
        ImVec2 armL(screen.x - 7.0f * z, t2Pos.y - 75.0f * z);
        ImVec2 armR(screen.x + 7.0f * z, t2Pos.y - 75.0f * z);
        drawList->AddLine(ImVec2(screen.x, t2Pos.y - 80.0f * z), armL, IM_COL32(90, 95, 100, 255), 3.0f * z);
        drawList->AddLine(ImVec2(screen.x, t2Pos.y - 80.0f * z), armR, IM_COL32(90, 95, 100, 255), 3.0f * z);
    }
}

void Renderer::drawTrees(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float baseZ = camera.getZoom();
    float z = baseZ * 3.5f; // Scale up all nature elements for a dense, lively mainland

    if (area.buildings.empty() && area.roads.empty()) return;

    // Calculate map bounds
    float minX = 0, maxX = 0, minY = 0, maxY = 0;
    if (!area.buildings.empty()) {
        minX = area.buildings[0].base[0].x; maxX = minX;
        minY = area.buildings[0].base[0].y; maxY = minY;
    }
    for (const Building& b : area.buildings) {
        for (const Vec2& p : b.base) {
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
        }
    }
    for (const Road& r : area.roads) {
        for (const Vec2& p : r.points) {
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
        }
    }

    minX -= 1200.0f; maxX += 1200.0f;
    minY -= 1200.0f; maxY += 1200.0f;

    auto isPointFree = [&](float px, float py) {
        auto pointLineDistanceSq = [](float x, float y, float x1, float y1, float x2, float y2) {
            float dx = x2 - x1;
            float dy = y2 - y1;
            float l2 = dx * dx + dy * dy;
            if (l2 == 0) return (x - x1)*(x - x1) + (y - y1)*(y - y1);
            float t = std::max(0.0f, std::min(1.0f, ((x - x1) * dx + (y - y1) * dy) / l2));
            float projX = x1 + t * dx;
            float projY = y1 + t * dy;
            return (x - projX)*(x - projX) + (y - projY)*(y - projY);
        };
        for (const Building& b : area.buildings) {
            if (b.base.empty()) continue;

            // 1. Point-in-polygon check (Ray-casting) to reject points deep inside the building
            bool inside = false;
            for (size_t i = 0, j = b.base.size() - 1; i < b.base.size(); j = i++) {
                if (((b.base[i].y > py) != (b.base[j].y > py)) &&
                    (px < (b.base[j].x - b.base[i].x) * (py - b.base[i].y) / (b.base[j].y - b.base[i].y) + b.base[i].x)) {
                    inside = !inside;
                }
            }
            if (inside) return false;

            // 2. Distance check to reject points too close to the outer walls
            for (size_t i = 0; i < b.base.size(); ++i) {
                size_t j = (i + 1) % b.base.size();
                if (pointLineDistanceSq(px, py, b.base[i].x, b.base[i].y, b.base[j].x, b.base[j].y) < 4000.0f) return false;
            }
        }
        for (const Road& r : area.roads) {
            if (r.points.empty()) continue;
            float minD = (r.lanes == 1 ? 40.0f : (r.lanes == 2 ? 65.0f : (r.lanes == 6 ? 115.0f : 90.0f))) * 0.5f + 18.0f;
            float minD2 = minD * minD + 1000.0f; // Add some margin
            for (size_t i = 0; i + 1 < r.points.size(); ++i) {
                if (pointLineDistanceSq(px, py, r.points[i].x, r.points[i].y, r.points[i+1].x, r.points[i+1].y) < minD2) return false;
            }
        }
        return true;
    };

    int maxTrees = 15000;
    int treeCount = 0;
    float gridStep = 75.0f;

    for (float x = minX; x <= maxX; x += gridStep) {
        for (float y = minY; y <= maxY; y += gridStep) {
            // Add pseudo-random jitter
            float px = x + ((int)(x * 13 + y * 37) % 80) - 40.0f;
            float py = y + ((int)(x * 19 + y * 41) % 80) - 40.0f;

            Vec2 worldPos(px, py);
            Vec2 screen = transformForView(worldPos, isometricMode);
            screen = applyCamera(screen, camera);

            // Frustum cull
            if (screen.x < -300 || screen.y < -400 || 
                screen.x > displaySize.x + 300 || screen.y > displaySize.y + 400) {
                continue;
            }

            if (isPointFree(px, py)) {
                int hash = std::abs((int)(px * 97 + py * 113)) % 100;

                if (hash < 30) {
                    // Grass tufts
                    ImU32 grassColor = IM_COL32(40, 180, 70, 200);
                    drawList->AddTriangleFilled(
                        ImVec2(screen.x, screen.y - 8.0f * z),
                        ImVec2(screen.x - 4.0f * z, screen.y + 2.0f * z),
                        ImVec2(screen.x + 4.0f * z, screen.y + 2.0f * z),
                        grassColor
                    );
                    drawList->AddTriangleFilled(
                        ImVec2(screen.x - 3.0f * z, screen.y - 5.0f * z),
                        ImVec2(screen.x - 6.0f * z, screen.y + 2.0f * z),
                        ImVec2(screen.x, screen.y + 2.0f * z),
                        IM_COL32(35, 160, 60, 200)
                    );
                    drawList->AddTriangleFilled(
                        ImVec2(screen.x + 3.0f * z, screen.y - 6.0f * z),
                        ImVec2(screen.x, screen.y + 2.0f * z),
                        ImVec2(screen.x + 6.0f * z, screen.y + 2.0f * z),
                        IM_COL32(45, 190, 80, 200)
                    );
                } else if (hash < 65) {
                    // Tree
                    drawList->AddCircleFilled(
                        ImVec2(screen.x + 4.0f * z, screen.y + 6.0f * z),
                        8.0f * z,
                        IM_COL32(0, 0, 0, 70)
                    );

                    int tVariety = hash % 3;

                    if (tVariety == 0) {
                        drawList->AddRectFilled(ImVec2(screen.x - 2.0f * z, screen.y + 4.0f * z), ImVec2(screen.x + 2.0f * z, screen.y + 13.0f * z), IM_COL32(110, 75, 35, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x, screen.y + 2.0f * z), 10.0f * z, IM_COL32(20, 110, 50, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 9.0f * z, IM_COL32(35, 150, 75, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x - 5.0f * z, screen.y - 3.0f * z), 6.5f * z, IM_COL32(45, 175, 85, 230));
                        drawList->AddCircleFilled(ImVec2(screen.x + 5.0f * z, screen.y + 2.0f * z), 6.5f * z, IM_COL32(25, 125, 60, 230));
                        drawList->AddCircleFilled(ImVec2(screen.x - 3.0f * z, screen.y + 1.0f * z), 1.2f * z, IM_COL32(235, 60, 60, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x + 2.0f * z, screen.y - 2.0f * z), 1.2f * z, IM_COL32(235, 60, 60, 240));
                    } else if (tVariety == 1) {
                        drawList->AddRectFilled(ImVec2(screen.x - 1.8f * z, screen.y + 4.0f * z), ImVec2(screen.x + 1.8f * z, screen.y + 14.0f * z), IM_COL32(90, 60, 30, 240));
                        drawList->AddTriangleFilled(ImVec2(screen.x, screen.y - 6.0f * z), ImVec2(screen.x - 9.0f * z, screen.y + 5.0f * z), ImVec2(screen.x + 9.0f * z, screen.y + 5.0f * z), IM_COL32(20, 95, 55, 240));
                        drawList->AddTriangleFilled(ImVec2(screen.x, screen.y - 12.0f * z), ImVec2(screen.x - 7.5f * z, screen.y - 1.0f * z), ImVec2(screen.x + 7.5f * z, screen.y - 1.0f * z), IM_COL32(25, 115, 65, 240));
                        drawList->AddTriangleFilled(ImVec2(screen.x, screen.y - 18.0f * z), ImVec2(screen.x - 5.5f * z, screen.y - 7.0f * z), ImVec2(screen.x + 5.5f * z, screen.y - 7.0f * z), IM_COL32(35, 140, 75, 240));
                    } else {
                        drawList->AddRectFilled(ImVec2(screen.x - 2.0f * z, screen.y + 4.0f * z), ImVec2(screen.x + 2.0f * z, screen.y + 13.0f * z), IM_COL32(110, 80, 50, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x, screen.y + 2.0f * z), 10.0f * z, IM_COL32(200, 70, 110, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 9.0f * z, IM_COL32(240, 110, 160, 240));
                        drawList->AddCircleFilled(ImVec2(screen.x - 5.0f * z, screen.y - 3.0f * z), 6.5f * z, IM_COL32(255, 150, 190, 230));
                        drawList->AddCircleFilled(ImVec2(screen.x + 5.0f * z, screen.y + 2.0f * z), 6.5f * z, IM_COL32(220, 90, 130, 230));
                        drawList->AddCircleFilled(ImVec2(screen.x - 2.0f * z, screen.y - 2.0f * z), 1.2f * z, IM_COL32(255, 255, 255, 240));
                    }
                    treeCount++;
                } else if (hash < 80) {
                    // Flowers
                    ImU32 fColor = (hash % 2 == 0) ? IM_COL32(255, 100, 150, 240) : IM_COL32(255, 200, 50, 240);
                    drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 2.5f * z, fColor);
                    drawList->AddCircleFilled(ImVec2(screen.x - 2.0f * z, screen.y + 2.0f * z), 2.0f * z, fColor);
                    drawList->AddCircleFilled(ImVec2(screen.x + 2.0f * z, screen.y + 2.0f * z), 2.0f * z, fColor);
                    drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 1.0f * z, IM_COL32(255, 255, 255, 240));
                } else if (hash < 85) {
                    // Park Bench
                    drawList->AddRectFilled(ImVec2(screen.x - 5.0f * z, screen.y - 2.0f * z), ImVec2(screen.x + 7.0f * z, screen.y + 4.0f * z), IM_COL32(0, 0, 0, 50));
                    drawList->AddRectFilled(ImVec2(screen.x - 6.0f * z, screen.y - 3.0f * z), ImVec2(screen.x + 6.0f * z, screen.y + 1.0f * z), IM_COL32(140, 90, 50, 255), 1.0f);
                    drawList->AddRectFilled(ImVec2(screen.x - 6.0f * z, screen.y - 6.0f * z), ImVec2(screen.x + 6.0f * z, screen.y - 3.0f * z), IM_COL32(120, 70, 40, 255), 1.0f);
                }

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

void Renderer::drawSkyBackground(const LiveContextEngine& liveContext) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float time = static_cast<float>(ImGui::GetTime());

    // 1. Celestial Bodies
    if (liveContext.isSunnyMode()) {
        // Draw Sun
        ImVec2 sunPos(displaySize.x * 0.85f, displaySize.y * 0.15f);
        // Glowing aura
        for (int i = 5; i > 0; --i) {
            float radius = 40.0f + i * 15.0f;
            int alpha = 40 - i * 7;
            drawList->AddCircleFilled(sunPos, radius, IM_COL32(255, 230, 100, alpha));
        }
        // Core
        drawList->AddCircleFilled(sunPos, 40.0f, IM_COL32(255, 240, 180, 255));
    } else if (liveContext.isNightMode()) {
        // Draw Stars
        for (int i = 0; i < 150; ++i) {
            // Pseudo-random deterministic placement
            float x = fmod(static_cast<float>(i * 3821), displaySize.x);
            float y = fmod(static_cast<float>(i * 7493), displaySize.y * 0.6f); // mostly upper half
            
            // Twinkle effect
            float offset = static_cast<float>(i);
            float alphaMod = (sin(time * 2.0f + offset) + 1.0f) * 0.5f; // 0 to 1
            int alpha = static_cast<int>(100 + 155 * alphaMod);
            
            drawList->AddCircleFilled(ImVec2(x, y), 1.0f + (i % 3) * 0.5f, IM_COL32(255, 255, 255, alpha));
        }

        // Draw Moon (Crescent using two circles or just full moon)
        ImVec2 moonPos(displaySize.x * 0.15f, displaySize.y * 0.2f);
        // Glow
        for (int i = 3; i > 0; --i) {
            drawList->AddCircleFilled(moonPos, 35.0f + i * 10.0f, IM_COL32(200, 220, 255, 20 - i * 5));
        }
        drawList->AddCircleFilled(moonPos, 35.0f, IM_COL32(220, 240, 255, 255));
        
        // Crater marks for realism
        drawList->AddCircleFilled(ImVec2(moonPos.x - 10, moonPos.y - 5), 8.0f, IM_COL32(180, 200, 220, 150));
        drawList->AddCircleFilled(ImVec2(moonPos.x + 12, moonPos.y + 8), 12.0f, IM_COL32(180, 200, 220, 100));
        drawList->AddCircleFilled(ImVec2(moonPos.x + 5, moonPos.y - 15), 6.0f, IM_COL32(180, 200, 220, 120));
    }

    // 2. Procedural Clouds (All modes)
    ImU32 cloudColor;
    if (liveContext.isNightMode()) {
        cloudColor = IM_COL32(60, 65, 80, 120);
    } else if (liveContext.isRainMode()) {
        cloudColor = IM_COL32(120, 130, 140, 180);
    } else {
        cloudColor = IM_COL32(255, 255, 255, 160);
    }

    int numClouds = 8;
    for (int c = 0; c < numClouds; ++c) {
        float speed = 15.0f + (c % 3) * 5.0f;
        float base_y = 50.0f + (c * 67) % 200;
        float offset_x = static_cast<float>(c * 345);
        
        // Wrap around using fmod
        float x = fmod(offset_x + time * speed, displaySize.x + 300.0f) - 150.0f;
        
        // Cloud composition (metaballs approach)
        drawList->AddCircleFilled(ImVec2(x, base_y), 40.0f, cloudColor);
        drawList->AddCircleFilled(ImVec2(x + 35.0f, base_y + 10.0f), 30.0f, cloudColor);
        drawList->AddCircleFilled(ImVec2(x - 35.0f, base_y + 5.0f), 35.0f, cloudColor);
        drawList->AddCircleFilled(ImVec2(x + 15.0f, base_y - 20.0f), 35.0f, cloudColor);
    }
}

void Renderer::drawForegroundBirds(const Camera2D& camera) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float time = static_cast<float>(ImGui::GetTime());
    float z = camera.getZoom() * 2.8f; // Greatly increased size factor
    
    // Draw 8 flocks of birds
    for (int flock = 0; flock < 8; ++flock) {
        float speed = 40.0f + flock * 15.0f;
        float start_y = 50.0f + flock * 80.0f; // Spread out across the sky
        float base_x = fmod(time * speed + flock * 350.0f, displaySize.x + 400.0f) - 200.0f;
        
        int numBirds = 3 + (flock % 4);
        
        for (int b = 0; b < numBirds; ++b) {
            float bx = base_x - b * 18.0f * z;
            float by = start_y + b * 12.0f * z + sin(time * 1.5f + b) * 18.0f * z; // Flight bobbing
            
            // Wing animation
            float wingOffset = sin(time * 12.0f + b) * 10.0f * z;
            
            ImVec2 center(bx, by);
            ImVec2 leftWing(bx - 12.0f * z, by - wingOffset);
            ImVec2 rightWing(bx + 12.0f * z, by - wingOffset);
            
            // Draw V shape for bird
            drawList->AddLine(leftWing, center, IM_COL32(30, 30, 35, 230), 2.5f * z);
            drawList->AddLine(center, rightWing, IM_COL32(30, 30, 35, 230), 2.5f * z);
        }
    }
}

void Renderer::drawPedestriansAndPets(
    const CityArea& area,
    bool isometricMode,
    const Camera2D& camera
) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float time = ImGui::GetTime();
    float z = camera.getZoom();

    int pedCount = 0;
    const int maxPeds = 200;

    for (const Road& road : area.roads) {
        if (road.points.size() < 2) continue;

        for (size_t i = 0; i + 1 < road.points.size(); i++) {
            Vec2 p1 = road.points[i];
            Vec2 p2 = road.points[i + 1];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float length = std::sqrt(dx * dx + dy * dy);

            if (length < 60.0f) continue;

            float nx = -dy / length;
            float ny = dx / length;

            // Spawn 2 entities per segment
            for (int e = 0; e < 2; e++) {
                // Offset calculation for movement
                float speed = 20.0f + ((i + e) % 15);
                float duration = length / speed;
                float progress = fmod(time + (i * 7.0f + e * 13.0f), duration) / duration;
                
                // One moves forward, one moves backward
                if (e == 1) progress = 1.0f - progress;

                Vec2 basePoint(
                    p1.x + dx * progress,
                    p1.y + dy * progress
                );

                float side = (e == 0) ? 1.0f : -1.0f;
                float roadWidthWorld = (road.lanes == 1 ? 40.0f : (road.lanes == 2 ? 65.0f : (road.lanes == 6 ? 115.0f : 90.0f)));
                float offsetDistWorld = roadWidthWorld * 0.5f + 9.0f;

                basePoint.x += nx * side * offsetDistWorld;
                basePoint.y += ny * side * offsetDistWorld;

                Vec2 screen = transformForView(basePoint, isometricMode);
                screen = applyCamera(screen, camera);

                if (screen.x < -20 || screen.y < -20 || 
                    screen.x > displaySize.x + 20 || screen.y > displaySize.y + 20) {
                    continue;
                }

                int entType = (i + e) % 3; // 0, 1: person, 2: pet

                float s = z * 1.6f; // Increase size a little bit

                // Entity shadow
                drawList->AddCircleFilled(ImVec2(screen.x, screen.y + 2.0f * s), 3.0f * s, IM_COL32(0, 0, 0, 80));

                if (entType < 2) {
                    // Draw Person
                    // Legs
                    float walkBob = sinf(time * speed * 0.5f) * 1.5f * s;
                    drawList->AddLine(ImVec2(screen.x - 1.5f*s, screen.y - 2.0f*s), ImVec2(screen.x - 1.5f*s - walkBob, screen.y + 1.0f*s), IM_COL32(50, 50, 60, 255), 1.5f*s);
                    drawList->AddLine(ImVec2(screen.x + 1.5f*s, screen.y - 2.0f*s), ImVec2(screen.x + 1.5f*s + walkBob, screen.y + 1.0f*s), IM_COL32(50, 50, 60, 255), 1.5f*s);
                    // Body
                    ImU32 shirtColor = (entType == 0) ? IM_COL32(200, 80, 80, 255) : IM_COL32(80, 120, 200, 255);
                    drawList->AddRectFilled(ImVec2(screen.x - 2.5f*s, screen.y - 7.0f*s), ImVec2(screen.x + 2.5f*s, screen.y - 2.0f*s), shirtColor, 1.0f*s);
                    // Head
                    drawList->AddCircleFilled(ImVec2(screen.x, screen.y - 9.0f*s), 2.0f*s, IM_COL32(240, 200, 160, 255));
                } else {
                    // Draw Pet (Dog)
                    float runBob = abs(sinf(time * speed)) * 2.0f * s;
                    // Body
                    drawList->AddRectFilled(ImVec2(screen.x - 3.5f*s, screen.y - 3.0f*s - runBob), ImVec2(screen.x + 3.5f*s, screen.y - 1.0f*s - runBob), IM_COL32(160, 110, 60, 255), 1.0f*s);
                    // Head
                    float headDir = (e == 0) ? 4.0f*s : -4.0f*s;
                    drawList->AddCircleFilled(ImVec2(screen.x + headDir, screen.y - 4.5f*s - runBob), 1.8f*s, IM_COL32(140, 90, 50, 255));
                    // Legs
                    drawList->AddLine(ImVec2(screen.x - 2.0f*s, screen.y - 1.0f*s - runBob), ImVec2(screen.x - 2.0f*s, screen.y + 1.0f*s), IM_COL32(140, 90, 50, 255), 1.2f*s);
                    drawList->AddLine(ImVec2(screen.x + 2.0f*s, screen.y - 1.0f*s - runBob), ImVec2(screen.x + 2.0f*s, screen.y + 1.0f*s), IM_COL32(140, 90, 50, 255), 1.2f*s);
                }

                pedCount++;
                if (pedCount >= maxPeds) return;
            }
        }
    }
}