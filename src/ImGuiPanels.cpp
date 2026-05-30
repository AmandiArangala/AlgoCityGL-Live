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

    ImGui::Text("Day 8 Status:");
    ImGui::BulletText("Camera pan added: W/A/S/D");
    ImGui::BulletText("Zoom added: Q/E");
    ImGui::BulletText("Camera reset added: C");
    ImGui::BulletText("Mini-map added");
    ImGui::BulletText("World-to-screen mapping introduced");

    ImGui::Separator();
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("W/A/S/D: Pan");
    ImGui::BulletText("Q/E: Zoom");
    ImGui::BulletText("C: Reset Camera");

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