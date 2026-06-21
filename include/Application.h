/**
 * @file Application.h
 * @brief Declares the Application class — the top-level orchestrator of AlgoCityGL Live.
 *
 * Application owns the GLFW window and every major subsystem.  Its run() method
 * drives the main loop:
 *   1. Poll input events (keyboard pan/zoom/rotate).
 *   2. Build the ImGui frame (control panel).
 *   3. Tick simulation (vehicles + traffic lights).
 *   4. Render the scene (background → city area → ImGui overlay).
 *   5. Swap OpenGL buffers.
 */

#pragma once

#include <string>

#include "Renderer.h"
#include "ImGuiPanels.h"
#include "AreaManager.h"
#include "VehicleController.h"
#include "SignalController.h"
#include "Camera2D.h"
#include "LiveContextEngine.h"

// Forward-declare GLFWwindow to avoid pulling in the full GLFW header here.
struct GLFWwindow;

class Application {
public:
    /**
     * @brief Construct and fully initialize the application.
     * @param width   Initial window width in pixels.
     * @param height  Initial window height in pixels.
     * @param title   Window title bar string.
     * @throws std::runtime_error if GLFW/GLAD/ImGui initialization fails.
     */
    Application(int width, int height, const std::string& title);

    /** @brief Destructor — calls shutdown() to cleanly release all resources. */
    ~Application();

    /**
     * @brief Enter the main render loop.
     * Runs until the user closes the window or presses ESC.
     */
    void run();

private:
    // ── Window metadata ───────────────────────────────────────────────────────
    int width;           ///< Framebuffer width (pixels).
    int height;          ///< Framebuffer height (pixels).
    std::string title;   ///< Window title.

    GLFWwindow* window;  ///< Raw GLFW window handle (null until initialize() succeeds).

    // ── Subsystems ────────────────────────────────────────────────────────────
    Renderer           renderer;           ///< Handles all pixel-buffer & ImGui draw-list rendering.
    ImGuiPanels        imguiPanels;        ///< The floating control panel (algorithms, area selector, etc.).
    AreaManager        areaManager;        ///< Parses and stores the active city-area JSON.
    VehicleController  vehicleController;  ///< Owns every vehicle; ticks movement each frame.
    SignalController   signalController;   ///< Runs traffic-light state machines (Red→Green→Yellow).
    Camera2D           camera;             ///< 2D viewport: pan, zoom, and rotation.
    LiveContextEngine  liveContextEngine;  ///< Active weather/context mode (Sunny, Rain, Night …).

    // ── Private helpers ───────────────────────────────────────────────────────

    /**
     * @brief Set up GLFW, GLAD, OpenGL viewport, ImGui, and the Renderer.
     * @return true on success, false on any initialization failure.
     */
    bool initialize();

    /** @brief Tear down ImGui, GLFW window, and GLFW library. */
    void shutdown();

    /**
     * @brief Read keyboard state and apply camera transformations.
     * Called once per frame before rendering.
     * Keys: W/A/S/D = pan, Q/E = zoom, Z/X = rotate, C = reset.
     */
    void processInput();

    /**
     * @brief Map the combo-box area index to a data file path.
     * @param selectedArea  Index from ImGuiPanels (0-3).
     * @return Relative path to the corresponding JSON file under data/.
     */
    std::string getAreaFilePath(int selectedArea) const;
};