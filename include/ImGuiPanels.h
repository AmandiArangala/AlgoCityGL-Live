/**
 * @file ImGuiPanels.h
 * @brief Declares ImGuiPanels — the floating control panel rendered with Dear ImGui.
 *
 * The panel exposes:
 *  - Simulation play/pause toggle.
 *  - Area selector (combo box) + "Load Area" button.
 *  - View mode selector (Top-Down 2D vs 2.5D Isometric).
 *  - Live context / weather mode selector.
 *  - Line-drawing algorithm selector (DDA vs Bresenham).
 *  - Algorithm X-Ray mode toggle (shows the raw pixel-buffer output).
 *  - Project status summary and camera help text.
 *
 * Interaction pattern
 * ───────────────────
 * Application::run() calls imguiPanels.render() each frame to build the widget tree.
 * It then queries the panel's state via getter methods to drive simulation and rendering.
 * The "Load Area" button uses a one-shot flag (consumeLoadAreaRequest) to avoid
 * re-loading the same file on every subsequent frame.
 */

#pragma once

class ImGuiPanels {
public:
    /**
     * @brief Build and submit the ImGui control panel for the current frame.
     *
     * Must be called between ImGui::NewFrame() and ImGui::Render() each frame.
     */
    void render();

    // ── State Getters ─────────────────────────────────────────────────────────

    /** @brief Returns true when Algorithm X-Ray mode is enabled. */
    bool getXRayMode() const;

    /** @brief Returns 0 for DDA, 1 for Bresenham. */
    int getSelectedLineAlgorithm() const;

    /** @brief Returns the index of the currently selected city area (0-3). */
    int getSelectedArea() const;

    /** @brief Returns the index of the selected weather/context mode (0-4). */
    int getSelectedWeatherMode() const;

    /** @brief Returns true when "2.5D Isometric" view is selected. */
    bool getIsometricMode() const;

    /** @brief Returns true when the simulation is in the playing state. */
    bool getIsPlaying() const;

    /**
     * @brief Consume a pending "Load Area" request.
     *
     * Returns true once when the user clicks the "Load Area" button, then
     * resets the internal flag to false.  This ensures the area is only
     * loaded once per button click, not every frame.
     */
    bool consumeLoadAreaRequest();

private:
    // ── Internal UI State ─────────────────────────────────────────────────────

    bool isPlaying         = false; ///< Whether the simulation is currently running.
    bool xrayMode          = false; ///< Whether to show the raw pixel-buffer overlay.
    bool loadAreaRequested = false; ///< One-shot flag set by the "Load Area" button.

    int selectedArea          = 2; ///< Default: Traffic Demo Location (index 2).
    int selectedWeather       = 0; ///< Default: Sunny (index 0).
    int selectedLineAlgorithm = 0; ///< Default: DDA (index 0).
    int selectedViewMode      = 0; ///< Default: Top-Down 2D (index 0).

    // ── Combo-box String Arrays ───────────────────────────────────────────────
    // These are null-terminated C-string arrays consumed by ImGui::Combo().

    /** City areas available for selection. */
    const char* areas[4] = {
        "University of Moratuwa",
        "Borella Junction",
        "Traffic Demo Location",
        "Random City Location"
    };

    /** Weather / live-context modes that affect rendering and vehicle speed. */
    const char* weatherModes[5] = {
        "Sunny",        ///< Normal rendering, full vehicle speed.
        "Rain",         ///< Rain particle effect, 65% vehicle speed.
        "Night",        ///< Dark sky, street-light glow, full speed.
        "Heavy Traffic",///< Orange tint, 45% vehicle speed.
        "Incident"      ///< Red incident marker, 55% vehicle speed.
    };

    /** Line-drawing algorithms that can be selected for the pixel buffer. */
    const char* lineAlgorithms[2] = {
        "DDA",       ///< Digital Differential Analyzer — float increments.
        "Bresenham"  ///< Bresenham's integer-only line algorithm.
    };

    /** Available rendering view modes. */
    const char* viewModes[2] = {
        "Top-Down 2D",   ///< Orthographic bird's-eye view.
        "2.5D Isometric" ///< Isometric projection with extruded building heights.
    };
};