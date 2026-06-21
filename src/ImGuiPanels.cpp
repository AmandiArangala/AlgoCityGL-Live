/**
 * @file ImGuiPanels.cpp
 * @brief Implements the Dear ImGui overlay for user interaction.
 *
 * This file creates the floating control panel seen on the right/bottom
 * of the screen. It maps ImGui widgets (buttons, combos, checkboxes)
 * to internal state variables that the Application reads each frame.
 */
#include "ImGuiPanels.h"
#include "imgui.h"

void ImGuiPanels::render() {
    // Begin a new ImGui window.
    ImGui::Begin("AlgoCityGL Live Control Panel");

    ImGui::Text("Area-Based 2.5D Smart Traffic Graphics Simulator");
    ImGui::Separator();

    ImGui::Text("Simulation Controls");

    // Play/Pause toggle button. The label changes based on the state.
    if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
        isPlaying = !isPlaying;
    }

    ImGui::Separator();

    ImGui::Text("Area Selection");
    // Combo box for selecting which JSON map to load.
    ImGui::Combo("Selected Area", &selectedArea, areas, IM_ARRAYSIZE(areas));

    // When clicked, sets a flag that Application consumes to trigger AreaManager.
    if (ImGui::Button("Load Area")) {
        loadAreaRequested = true;
    }

    ImGui::Separator();

    ImGui::Text("View Mode");
    // Toggles between Top-Down (0) and Isometric 2.5D (1).
    ImGui::Combo("City View", &selectedViewMode, viewModes, IM_ARRAYSIZE(viewModes));

    ImGui::Separator();

    ImGui::Text("Live Context Mode");
    // Sets weather/traffic scenarios (Sunny, Rain, Night, etc.).
    ImGui::Combo("Weather / Traffic Mode", &selectedWeather, weatherModes, IM_ARRAYSIZE(weatherModes));

    ImGui::Separator();

    ImGui::Text("Drawing Algorithm");
    // Selects rasterisation algorithm (DDA vs Bresenham).
    ImGui::Combo("Line Algorithm", &selectedLineAlgorithm, lineAlgorithms, IM_ARRAYSIZE(lineAlgorithms));

    // Toggles whether the raw algorithmic pixel buffer is drawn over the scene.
    ImGui::Checkbox("Algorithm X-Ray Mode", &xrayMode);

    ImGui::Separator();

    // Informational bullet list demonstrating project features.
    ImGui::Text("Overall Project Status:");
    ImGui::BulletText("Real city map loading completed");
    ImGui::BulletText("2.5D Isometric rendering active");
    ImGui::BulletText("Smart traffic simulation integrated");
    ImGui::BulletText("Multiple weather & context modes working");
    ImGui::BulletText("Algorithm X-Ray mode fully functional");
    ImGui::BulletText("Dynamic vehicle types & scaling implemented");

    ImGui::Separator();
    
    // Help text for camera controls.
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("W/A/S/D: Pan");
    ImGui::BulletText("Q/E: Zoom");
    ImGui::BulletText("Z/X: 360 Rotation");
    ImGui::BulletText("C: Reset Camera");

    // End the ImGui window.
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

int ImGuiPanels::getSelectedWeatherMode() const {
    return selectedWeather;
}