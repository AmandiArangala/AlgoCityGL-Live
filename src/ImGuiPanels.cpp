#include "ImGuiPanels.h"
#include "imgui.h"

void ImGuiPanels::render() {
    ImGui::Begin("AlgoCityGL Live Control Panel");

    ImGui::Text("Area-Based 2.5D Smart Traffic Graphics Simulator");
    ImGui::Separator();

    ImGui::Text("Simulation Controls");

    if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
        isPlaying = !isPlaying;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset")) {
        isPlaying = false;
        resetRequested = true;
    }

    ImGui::Separator();

    ImGui::Text("Area Selection");
    ImGui::Combo("Selected Area", &selectedArea, areas, IM_ARRAYSIZE(areas));

    if (ImGui::Button("Load Area")) {
        loadAreaRequested = true;
    }

    ImGui::Separator();

    ImGui::Text("View Mode");
    ImGui::Combo("City View", &selectedViewMode, viewModes, IM_ARRAYSIZE(viewModes));

    ImGui::Separator();

    ImGui::Text("Live Context Mode");
    ImGui::Combo("Weather / Traffic Mode", &selectedWeather, weatherModes, IM_ARRAYSIZE(weatherModes));

    ImGui::Separator();

    ImGui::Text("Drawing Algorithm");
    ImGui::Combo("Line Algorithm", &selectedLineAlgorithm, lineAlgorithms, IM_ARRAYSIZE(lineAlgorithms));

    ImGui::Checkbox("Algorithm X-Ray Mode", &xrayMode);

    ImGui::Separator();

    ImGui::Text("Day 7 Status:");
    ImGui::BulletText("Scan-line polygon filling added");
    ImGui::BulletText("Odd-even rule used for polygon fill");
    ImGui::BulletText("Horizontal edge handling added");
    ImGui::BulletText("Top-down buildings filled");
    ImGui::BulletText("Road thickness improved");
    ImGui::BulletText("Stopped vehicle color indicator added");

    ImGui::End();
}

bool ImGuiPanels::getXRayMode() const {
    return xrayMode;
}

int ImGuiPanels::getSelectedLineAlgorithm() const {
    return selectedLineAlgorithm;
}

int ImGuiPanels::getSelectedArea() const {
    return selectedArea;
}

bool ImGuiPanels::getIsometricMode() const {
    return selectedViewMode == 1;
}

bool ImGuiPanels::consumeLoadAreaRequest() {
    if (loadAreaRequested) {
        loadAreaRequested = false;
        return true;
    }

    return false;
}

bool ImGuiPanels::getIsPlaying() const {
    return isPlaying;
}

bool ImGuiPanels::consumeResetRequest() {
    if (resetRequested) {
        resetRequested = false;
        return true;
    }

    return false;
}