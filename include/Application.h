/**
 * @file Application.h
 * @brief Declares the Application class — the top-level orchestrator of AlgoCityGL Live.
 *
 * Application owns the GLFW window and every major subsystem.  Its run() method
 * drives the main loop:
 *   1. Poll keyboard input (pan / zoom / rotate the camera).
 *   2. Poll GLFW events (resize, close, etc.).
 *   3. Build and render the Dear ImGui control panel.
 *   4. Update traffic-signal and vehicle simulation.
 *   5. Render the city scene (or a demo test scene if no area is loaded).
 *   6. Swap the OpenGL back-buffer to the screen.
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

struct GLFWwindow;

class Application {
public:
    /**
     * @brief Constructs and initialises the application.
     * @param width   Width of the window in pixels.
     * @param height  Height of the window in pixels.
     * @param title   Text shown in the window title bar.
     * @throws std::runtime_error if GLFW or GLAD initialisation fails.
     */
    Application(int width, int height, const std::string& title);

    /** @brief Destructor — calls shutdown() to release GLFW and ImGui resources. */
    ~Application();

    /** @brief Starts the main render loop — returns when the user closes the window. */
    void run();

private:
    int width;           ///< Window width in pixels.
    int height;          ///< Window height in pixels.
    std::string title;   ///< Window title string.

    GLFWwindow* window;  ///< Raw pointer to the GLFW window (owned by this class).

    Renderer          renderer;           ///< Handles all OpenGL / ImGui drawing.
    ImGuiPanels       imguiPanels;        ///< Floating control panel (play/pause, area select, etc.).
    AreaManager       areaManager;        ///< Loads and stores the current city-area JSON.
    VehicleController vehicleController;  ///< Spawns and ticks all vehicle agents.
    SignalController  signalController;   ///< Runs the traffic-light state machine.
    Camera2D          camera;             ///< 2D viewport (pan / zoom / rotation).
    LiveContextEngine liveContextEngine;  ///< Active weather / traffic mode (sunny, rain, night…).

    /** @brief Creates the GLFW window, loads OpenGL via GLAD, and sets up ImGui. */
    bool initialize();

    /** @brief Tears down ImGui and GLFW in the correct order. */
    void shutdown();

    /** @brief Reads keyboard state every frame to pan, zoom, and rotate the camera. */
    void processInput();

    /**
     * @brief Maps the combo-box index chosen in the UI to the matching JSON file path.
     * @param selectedArea  Index from the \"Selected Area\" combo box (0–3).
     * @return Relative file path string understood by AreaManager.
     */
    std::string getAreaFilePath(int selectedArea) const;
};