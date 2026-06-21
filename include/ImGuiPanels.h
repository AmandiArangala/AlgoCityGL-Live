/**
 * @file ImGuiPanels.h
 * @brief Declares ImGuiPanels — the floating control panel rendered with Dear ImGui.
 *
 * The panel exposes:
 *  - Simulation play / pause toggle.
 *  - Area selector (combo box) + \"Load Area\" button.
 *  - View mode selector: Top-Down 2D or 2.5D Isometric.
 *  - Weather / context mode selector (Sunny, Rain, Night, Heavy Traffic, Incident).
 *  - Line algorithm selector: DDA or Bresenham.
 *  - Algorithm X-Ray mode toggle (draws raster lines on top of / under the scene).
 *  - Status bullet list and camera control hints.
 *
 * All state is stored as private member variables.  Getter methods expose
 * the current values to Application so it can drive the simulation and renderer.
 */
#pragma once

class ImGuiPanels {
public:
    /** @brief Builds and renders the entire control panel window each frame. */
    void render();

    bool getXRayMode()              const; ///< Returns true if X-Ray algorithm overlay is active.
    int  getSelectedLineAlgorithm() const; ///< Returns 0=DDA or 1=Bresenham.
    int  getSelectedArea()          const; ///< Returns the index of the chosen city area (0–3).
    int  getSelectedWeatherMode()   const; ///< Returns the index of the chosen weather mode (0–4).
    bool getIsometricMode()         const; ///< Returns true when \"2.5D Isometric\" view is selected.
    bool getIsPlaying()             const; ///< Returns true while simulation is running.

    /**
     * @brief Checks and clears the one-shot \"Load Area\" request flag.
     * @return true once per button press; false every other frame.
     */
    bool consumeLoadAreaRequest();

private:
    bool isPlaying         = false; ///< Play/pause toggle state.
    bool xrayMode          = false; ///< X-Ray algorithm visualisation toggle.
    bool loadAreaRequested = false; ///< One-shot flag set when \"Load Area\" is pressed.

    int selectedArea          = 2; ///< Index into the areas[] array (default: Traffic Demo).
    int selectedWeather       = 0; ///< Index into the weatherModes[] array (default: Sunny).
    int selectedLineAlgorithm = 0; ///< Index into the lineAlgorithms[] array (default: DDA).
    int selectedViewMode      = 0; ///< Index into the viewModes[] array (default: Top-Down 2D).

    /// City area options shown in the combo box.
    const char* areas[4] = {
        "University of Moratuwa",
        "Borella Junction",
        "Traffic Demo Location",
        "Random City Location"
    };

    /// Weather / traffic context options shown in the combo box.
    const char* weatherModes[5] = {
        "Sunny",
        "Rain",
        "Night",
        "Heavy Traffic",
        "Incident"
    };

    /// Rasterisation line algorithm options.
    const char* lineAlgorithms[2] = {
        "DDA",
        "Bresenham"
    };

    /// Camera perspective options.
    const char* viewModes[2] = {
        "Top-Down 2D",
        "2.5D Isometric"
    };
};