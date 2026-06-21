/**
 * @file ImGuiPanels.cpp
 * @brief Implements the ImGuiPanels control panel using Dear ImGui widgets.
 *
 * render() is called once per frame and builds the "AlgoCityGL Live Control Panel"
 * floating window.  Every widget (combo box, button, checkbox) writes its output
 * into a private member variable that the Application then reads via the getter
 * methods.
 *
 * Widget → State mapping
 * ─────────────────────
 *  Button  "Play/Pause"  →  isPlaying  (toggled on press)
 *  Combo   "Selected Area" →  selectedArea
 *  Button  "Load Area"   →  loadAreaRequested (one-shot flag)
 *  Combo   "City View"   →  selectedViewMode
 *  Combo   "Weather / Traffic Mode" → selectedWeather
 *  Combo   "Line Algorithm" → selectedLineAlgorithm
 *  Checkbox "Algorithm X-Ray Mode" → xrayMode
 */

#include "ImGuiPanels.h"
#include "imgui.h"

// ─────────────────────────────────────────────────────────────────────────────
// ImGuiPanels::render — Build the control panel for the current frame.
// ─────────────────────────────────────────────────────────────────────────────

void ImGuiPanels::render() {
    // Begin a new ImGui window.  The window is freely draggable by the user.
    ImGui::Begin("AlgoCityGL Live Control Panel");

    // ── Header ────────────────────────────────────────────────────────────────
    ImGui::Text("Area-Based 2.5D Smart Traffic Graphics Simulator");
    ImGui::Separator();

    // ── Simulation Play / Pause ───────────────────────────────────────────────
    ImGui::Text("Simulation Controls");

    // The button label changes dynamically to reflect the current state.
    // Clicking toggles isPlaying, which controls whether the main loop ticks
    // the SignalController and VehicleController each frame.
    if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
        isPlaying = !isPlaying;
    }

    ImGui::Separator();

    // ── Area Selection ────────────────────────────────────────────────────────
    ImGui::Text("Area Selection");

    // Combo box: user picks which JSON city area to load.
    // selectedArea stores the index into the `areas` array.
    ImGui::Combo("Selected Area", &selectedArea, areas, IM_ARRAYSIZE(areas));

    // Clicking "Load Area" sets the one-shot flag.
    // Application::run() calls consumeLoadAreaRequest() to detect this.
    if (ImGui::Button("Load Area")) {
        loadAreaRequested = true;
    }

    ImGui::Separator();

    // ── View Mode ─────────────────────────────────────────────────────────────
    ImGui::Text("View Mode");

    // Selects between Top-Down 2D and 2.5D Isometric rendering.
    // getIsometricMode() returns (selectedViewMode == 1).
    ImGui::Combo("City View", &selectedViewMode, viewModes, IM_ARRAYSIZE(viewModes));

    ImGui::Separator();

    // ── Live Context / Weather Mode ───────────────────────────────────────────
    ImGui::Text("Live Context Mode");

    // Selects the active environment simulation: Sunny, Rain, Night,
    // Heavy Traffic, or Incident.  The mode drives both rendering effects
    // and the vehicle-speed multiplier.
    ImGui::Combo("Weather / Traffic Mode", &selectedWeather, weatherModes, IM_ARRAYSIZE(weatherModes));

    ImGui::Separator();

    // ── Drawing Algorithm ─────────────────────────────────────────────────────
    ImGui::Text("Drawing Algorithm");

    // Chooses which rasterization algorithm is used for the pixel buffer:
    //   0 = DDA (Digital Differential Analyzer) — uses floating-point increments.
    //   1 = Bresenham — uses only integer arithmetic (faster, no rounding error).
    ImGui::Combo("Line Algorithm", &selectedLineAlgorithm, lineAlgorithms, IM_ARRAYSIZE(lineAlgorithms));

    // When enabled, the Renderer draws the raw pixel-buffer output on top of the
    // scene so the user can see exactly which pixels the chosen algorithm touches.
    ImGui::Checkbox("Algorithm X-Ray Mode", &xrayMode);

    ImGui::Separator();

    // ── Project Status Summary ────────────────────────────────────────────────
    // Static informational text; not driven by live state.
    ImGui::Text("Overall Project Status:");
    ImGui::BulletText("Real city map loading completed");
    ImGui::BulletText("2.5D Isometric rendering active");
    ImGui::BulletText("Smart traffic simulation integrated");
    ImGui::BulletText("Multiple weather & context modes working");
    ImGui::BulletText("Algorithm X-Ray mode fully functional");
    ImGui::BulletText("Dynamic vehicle types & scaling implemented");

    ImGui::Separator();

    // ── Camera Controls Reference ─────────────────────────────────────────────
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("W/A/S/D: Pan");
    ImGui::BulletText("Q/E: Zoom");
    ImGui::BulletText("Z/X: 360 Rotation");
    ImGui::BulletText("C: Reset Camera");

    ImGui::End(); // Finalize and close the window definition.
}

// ─────────────────────────────────────────────────────────────────────────────
// State Getters
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Returns the current state of the X-Ray toggle. */
bool ImGuiPanels::getXRayMode() const {
    return xrayMode;
}

/** @brief Returns the index of the selected line algorithm (0=DDA, 1=Bresenham). */
int ImGuiPanels::getSelectedLineAlgorithm() const {
    return selectedLineAlgorithm;
}

/** @brief Returns the index of the selected city area (0–3). */
int ImGuiPanels::getSelectedArea() const {
    return selectedArea;
}

/**
 * @brief Returns true when "2.5D Isometric" is selected.
 *
 * Implemented as (selectedViewMode == 1) so the Renderer does not need to
 * know the internal index values.
 */
bool ImGuiPanels::getIsometricMode() const {
    return selectedViewMode == 1;
}

/**
 * @brief Consume and reset the "Load Area" one-shot flag.
 *
 * Returns true once per button press, then false on every subsequent
 * frame until the button is pressed again.
 */
bool ImGuiPanels::consumeLoadAreaRequest() {
    if (loadAreaRequested) {
        loadAreaRequested = false; // Reset immediately so we don't re-load next frame.
        return true;
    }

    return false;
}

/** @brief Returns the current play/pause state. */
bool ImGuiPanels::getIsPlaying() const {
    return isPlaying;
}

/** @brief Returns the index of the selected weather/context mode (0–4). */
int ImGuiPanels::getSelectedWeatherMode() const {
    return selectedWeather;
}