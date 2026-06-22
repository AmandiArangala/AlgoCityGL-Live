/**
 * @file Application.cpp
 * @brief Implements the Application class: window setup, main loop, input, and shutdown.
 *
 * Initialization sequence (Application::initialize)
 * ──────────────────────────────────────────────────
 *  1. glfwInit()          — start the GLFW library.
 *  2. glfwCreateWindow()  — open an OpenGL 3.3 Core Profile window.
 *  3. gladLoadGL()        — load all OpenGL function pointers via GLAD.
 *  4. ImGui setup         — create context, pick dark theme, bind GLFW+OpenGL3 backends.
 *  5. renderer.initialize() — upload the pixel-buffer texture to the GPU.
 *
 * Main loop (Application::run)
 * ────────────────────────────
 *  Each iteration:
 *    a. processInput()          — keyboard → camera
 *    b. glfwPollEvents()        — dispatch OS events (resize, close …)
 *    c. ImGui frame             — build control panel
 *    d. Simulation tick         — signals + vehicles
 *    e. Render background       — clear + sky gradient
 *    f. Render city or test scene
 *    g. ImGui render            — draw control panel on top
 *    h. glfwSwapBuffers()       — present the finished frame
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Application.h"
#include "SignalController.h"

#include <stdexcept>
#include <iostream>
#include <string>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

Application::Application(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr) {
    // Delegate all setup work to initialize().  Throw if anything fails so that
    // main() can catch it and print a helpful error message.
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize application");
    }
}

Application::~Application() {
    // Always clean up even if the user closes the window mid-session.
    shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// Application::initialize
// ─────────────────────────────────────────────────────────────────────────────

bool Application::initialize() {
    // ── Step 1: Start GLFW ───────────────────────────────────────────────────
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // ── Step 2: Request an OpenGL 3.3 Core Profile context ───────────────────
    // Core Profile removes deprecated features; keeps the API clean and portable.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ── Step 3: Create the OS window + OpenGL context ────────────────────────
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window); // Make this window's context the active one.
    glfwSwapInterval(1);            // Enable VSync (swap once per monitor refresh).

    // ── Step 4: Load OpenGL function pointers via GLAD ───────────────────────
    // GLAD queries the driver for each GL function address at runtime.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Set the initial OpenGL viewport to match the window size.
    glViewport(0, 0, width, height);

    // Register a framebuffer-resize callback so the viewport stays correct
    // when the user drags the window border.
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
    });

    // ── Step 5: Initialize Dear ImGui ────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io; // Suppress "unused variable" warning; io is used implicitly by ImGui.

    ImGui::StyleColorsDark(); // Use the dark theme for the control panel.

    // Bind ImGui to GLFW (input) and OpenGL 3.3 (rendering).
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ── Step 6: Initialize the Renderer ──────────────────────────────────────
    // Creates the GPU texture that the pixel buffer is uploaded to each frame.
    renderer.initialize();

    std::cout << "AlgoCityGL Live initialized successfully" << std::endl;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Application::run — Main Render Loop
// ─────────────────────────────────────────────────────────────────────────────

void Application::run() {
    // Loop until GLFW is told to close (e.g., user presses the X button or ESC).
    while (!glfwWindowShouldClose(window)) {

        // ── Input ─────────────────────────────────────────────────────────────
        processInput(); // Handle keyboard-driven camera movement.

        // ── OS Events ─────────────────────────────────────────────────────────
        glfwPollEvents(); // Process window/input events from the OS event queue.

        // ── ImGui Frame Begin ─────────────────────────────────────────────────
        // These three calls must always be in this order before any ImGui::* UI calls.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── Build the Control Panel ───────────────────────────────────────────
        imguiPanels.render(); // Fills the ImGui command buffer with the panel widgets.

        // Sync the live-context mode (weather) with what the user selected in the panel.
        liveContextEngine.setMode(imguiPanels.getSelectedWeatherMode());

        // ── Handle "Load Area" Button Press ───────────────────────────────────
        // consumeLoadAreaRequest() returns true exactly once per button press,
        // then resets the flag, preventing repeated reloads.
        if (imguiPanels.consumeLoadAreaRequest()) {
            std::string filePath = getAreaFilePath(imguiPanels.getSelectedArea());

            // Attempt to load the JSON file.
            if (areaManager.loadAreaFromFile(filePath)) {
                const CityArea& area = areaManager.getCurrentArea();

                // Spawn vehicles along every route in the newly loaded area.
                vehicleController.initializeFromArea(area);
                // Initialize traffic lights from the area's static config.
                signalController.initializeFromArea(area);

                // Calculate the center of the loaded area to use as the camera's rotation pivot.
                // We scan all road waypoints to find the bounding box, then use its centre.
                float minX = 999999.0f, minY = 999999.0f, maxX = -999999.0f, maxY = -999999.0f;
                for (const auto& r : area.roads) {
                    for (const auto& pt : r.points) {
                        if (pt.x < minX) minX = pt.x;
                        if (pt.y < minY) minY = pt.y;
                        if (pt.x > maxX) maxX = pt.x;
                        if (pt.y > maxY) maxY = pt.y;
                    }
                }
                // Set the pivot so the city rotates around its own centre, not the origin.
                camera.setRotationCenter(Vec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f));
            }
        }

        // ── Simulation Tick ───────────────────────────────────────────────────
        // Only advance traffic lights when the simulation is playing.
        // Fixed timestep: 1/60 second per frame (matches 60 FPS VSync).
        if (imguiPanels.getIsPlaying()) {
            // Update traffic lights first
            signalController.update(1.0f / 60.0f);
        }

        // Vehicles are always updated (even when paused, deltaTime=0 has no effect)
        // so they can receive the latest traffic light states.
        // The speed multiplier comes from the weather mode (rain → slower vehicles).
        vehicleController.update(
            (1.0f / 60.0f) * liveContextEngine.getVehicleSpeedMultiplier(),
            imguiPanels.getIsPlaying(),
            signalController.getTrafficLights()
        );

        // ── Render ────────────────────────────────────────────────────────────
        renderer.renderBackground(); // Clear the screen and draw the sky gradient.

        if (areaManager.hasLoadedArea()) {
            // Render the live city area with all vehicles, traffic lights, and effects.
            renderer.renderCityArea(
                areaManager.getCurrentArea(),
                vehicleController.getVehicles(),
                signalController.getTrafficLights(),
                liveContextEngine,
                imguiPanels.getXRayMode(),
                imguiPanels.getSelectedLineAlgorithm(),
                imguiPanels.getIsometricMode(),
                camera
            );
        } else {
            // No area loaded yet — show the Day 2 algorithm test scene
            // (demonstrates DDA/Bresenham lines and fill/clipping algorithms).
            renderer.renderDay2TestScene(
                imguiPanels.getXRayMode(),
                imguiPanels.getSelectedLineAlgorithm()
            );
        }

        // ── ImGui Frame End ───────────────────────────────────────────────────
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Flush ImGui to OpenGL.

        // ── Present Frame ─────────────────────────────────────────────────────
        glfwSwapBuffers(window); // Swap the back buffer to the screen.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Application::processInput — Keyboard Camera Control
// ─────────────────────────────────────────────────────────────────────────────

void Application::processInput() {
    // ESC closes the application.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float panSpeed = 6.0f; // Pixels per frame the camera pans.

    // W / S — pan the view up / down (positive Y = down in screen space).
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.pan(0.0f, panSpeed);   // Shift everything downward → appears to move up.
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.pan(0.0f, -panSpeed);
    }

    // A / D — pan the view left / right.
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.pan(panSpeed, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.pan(-panSpeed, 0.0f);
    }

    // E / Q — zoom in / out (zoom is clamped by Camera2D to [0.4 … 3.0]).
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.zoomIn();
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.zoomOut();
    }

    // C — reset the camera to its default state (no offset, zoom=1, no rotation).
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera.reset();
    }

    // Z / X — rotate the city view anti-clockwise / clockwise.
    // rotationSpeed of 1.0 combined with 0.016 s (≈ 1 frame at 60 FPS) gives
    // ~1.5 degrees of rotation per frame when held down.
    float rotationSpeed = 1.0f; // Could use deltaTime if available, but processInput is called every frame
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        camera.rotateLeft(0.016f * rotationSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        camera.rotateRight(0.016f * rotationSpeed);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Application::shutdown — Clean-up Resources
// ─────────────────────────────────────────────────────────────────────────────

void Application::shutdown() {
    // Shut down ImGui backends in reverse initialization order.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy the GLFW window and then terminate the GLFW library.
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

// ─────────────────────────────────────────────────────────────────────────────
// Application::getAreaFilePath — Map combo-box index to data file
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns the file path for the city-area JSON selected in the UI.
 *
 * Paths are relative to the build/ directory (where the executable runs).
 * Index mapping:
 *   0 → University of Moratuwa (real OSM data)
 *   1 → Borella Junction (real OSM data)
 *   2 → Traffic Demo Location (hand-crafted test area)
 *   3 → Randomly generated city
 */
std::string Application::getAreaFilePath(int selectedArea) const {
    switch (selectedArea) {
        case 0:  return "../data/moratuwa_area_real.json";
        case 1:  return "../data/borella_area_real.json";
        case 2:  return "../data/demo_traffic_area.json";
        case 3:  return "../data/random_city_area.json";
        default: return "../data/moratuwa_area_real.json";
    }
}