/**
 * @file main.cpp
 * @brief Entry point for AlgoCityGL Live — a 2.5D Smart Traffic Graphics Simulator.
 *
 * This file creates and runs the main Application object. It wraps startup in a
 * try/catch block so that any initialization failure (e.g., OpenGL context creation,
 * shader compilation) is reported to the user before the program exits.
 *
 * Architecture overview:
 *  main.cpp
 *    └── Application        (manages the GLFW window + main loop)
 *          ├── Renderer           (pixel-buffer & ImGui draw-list rendering)
 *          ├── ImGuiPanels        (control panel UI)
 *          ├── AreaManager        (loads city map JSON)
 *          ├── VehicleController  (spawns & ticks all vehicles)
 *          ├── SignalController   (manages traffic-light state machines)
 *          ├── Camera2D           (pan / zoom / rotate viewport)
 *          └── LiveContextEngine  (weather/context mode — rain, night, etc.)
 */

#include "Application.h"
#include <iostream>

int main() {
    try {
        // Create the application with a 1280×720 window.
        // The constructor calls Application::initialize() internally and throws
        // std::runtime_error if GLFW, GLAD, or ImGui setup fails.
        Application app(1280, 720, "AlgoCityGL Live");

        // Enter the main render loop (runs until the window is closed or ESC is pressed).
        app.run();
    } catch (const std::exception& e) {
        // Print the error message and exit with a failure code.
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}