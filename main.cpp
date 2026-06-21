/**
 * @file main.cpp
 * @brief Entry point for AlgoCityGL Live — a 2.5D Smart Traffic Graphics Simulator.
 *
 * Responsibilities:
 *   1. Creates an Application object (which owns the OpenGL window and all subsystems).
 *   2. Calls app.run() to start the main render loop.
 *   3. Catches any std::exception thrown during initialisation or the loop
 *      and prints it to stderr before exiting with error code -1.
 */
#include "Application.h"
#include <iostream>

int main() {
    try {
        // Construct the application with a 1280x720 window.
        // The constructor throws std::runtime_error if GLFW / GLAD initialisation fails.
        Application app(1280, 720, "AlgoCityGL Live");

        // Enter the main render loop — blocks until the user closes the window.
        app.run();
    } catch (const std::exception& e) {
        // Print any fatal error and exit with a non-zero code.
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }

    return 0; // Normal exit.
}