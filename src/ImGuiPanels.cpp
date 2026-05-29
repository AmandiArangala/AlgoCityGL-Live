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
    }

    ImGui::Separator();

    ImGui::Text("Area Selection");
    ImGui::Combo("Selected Area", &selectedArea, areas, IM_ARRAYSIZE(areas));

    if (ImGui::Button("Load Area")) {
        loadAreaRequested = true;
    }

    ImGui::Separator();

    ImGui::Text("Live Context Mode");
    ImGui::Combo("Weather / Traffic Mode", &selectedWeather, weatherModes, IM_ARRAYSIZE(weatherModes));

    ImGui::Separator();

    ImGui::Text("Drawing Algorithm");
    ImGui::Combo("Line Algorithm", &selectedLineAlgorithm, lineAlgorithms, IM_ARRAYSIZE(lineAlgorithms));

    ImGui::Checkbox("Algorithm X-Ray Mode", &xrayMode);

    ImGui::Separator();

    ImGui::Text("Day 3 Status:");
    ImGui::BulletText("Area JSON loading added");
    ImGui::BulletText("Road data loading added");
    ImGui::BulletText("Building data loading added");
    ImGui::BulletText("Traffic signal data loading added");
    ImGui::BulletText("Routes loading added");

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

bool ImGuiPanels::consumeLoadAreaRequest() {
    if (loadAreaRequested) {
        loadAreaRequested = false;
        return true;
    }

    return false;
}