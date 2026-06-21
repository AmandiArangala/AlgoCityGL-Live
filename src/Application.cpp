/**
 * @file Application.cpp
 * @brief Implements the main Application class, the core orchestrator of AlgoCityGL.
 *
 * This file manages the primary run loop, window lifecycle, input processing,
 * and orchestration between the Renderer, live simulation controllers, and UI.
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

Application::Application(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr) {
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize application");
    }
}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Require OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the main window.
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    // Enable v-sync
    glfwSwapInterval(1);

    // Initialize GLAD to load OpenGL function pointers.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Set the initial OpenGL viewport size.
    glViewport(0, 0, width, height);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
    });

    // Initialize Dear ImGui context and styling.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    renderer.initialize();

    std::cout << "AlgoCityGL Live initialized successfully" << std::endl;

    return true;
}

void Application::run() {
    // Main render and simulation loop.
    while (!glfwWindowShouldClose(window)) {
        processInput(); // Handle WASD, Zoom, etc.

        glfwPollEvents(); // Process window events.

        // Start a new ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render the UI panels and update configuration states.
        imguiPanels.render();
        liveContextEngine.setMode(imguiPanels.getSelectedWeatherMode());

        // Check if the user requested a new area to load.
        if (imguiPanels.consumeLoadAreaRequest()) {
            std::string filePath = getAreaFilePath(imguiPanels.getSelectedArea());

            // Attempt to load the JSON file.
            if (areaManager.loadAreaFromFile(filePath)) {
                const CityArea& area = areaManager.getCurrentArea();
                
                // Re-initialise the simulation controllers with the new data.
                vehicleController.initializeFromArea(area);
                signalController.initializeFromArea(area);
                
                // Calculate the center of the loaded area to use as the camera's rotation pivot.
                // This ensures Z/X rotation always spins around the centre of the map.
                float minX = 999999.0f, minY = 999999.0f, maxX = -999999.0f, maxY = -999999.0f;
                for (const auto& r : area.roads) {
                    for (const auto& pt : r.points) {
                        if (pt.x < minX) minX = pt.x;
                        if (pt.y < minY) minY = pt.y;
                        if (pt.x > maxX) maxX = pt.x;
                        if (pt.y > maxY) maxY = pt.y;
                    }
                }
                camera.setRotationCenter(Vec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f));
            }
        }

        // Update Simulation (only if playing)
        if (imguiPanels.getIsPlaying()) {
            // Update traffic lights first
            signalController.update(1.0f / 60.0f);
        }

        // Update vehicles (they use the context engine for weather-based speed penalties).
        vehicleController.update(
            (1.0f / 60.0f) * liveContextEngine.getVehicleSpeedMultiplier(),
            imguiPanels.getIsPlaying(),
            signalController.getTrafficLights()
        );

        // Render everything.
        renderer.renderBackground();

        if (areaManager.hasLoadedArea()) {
            // Draw the fully simulated city area.
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
            // Draw the fallback test scene if no area is loaded.
            renderer.renderDay2TestScene(
                imguiPanels.getXRayMode(),
                imguiPanels.getSelectedLineAlgorithm()
            );
        }

        // Render ImGui over the OpenGL frame.
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the front and back buffers to present the frame.
        glfwSwapBuffers(window);
    }
}

void Application::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float panSpeed = 6.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.pan(0.0f, panSpeed);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.pan(0.0f, -panSpeed);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.pan(panSpeed, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.pan(-panSpeed, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.zoomIn();
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.zoomOut();
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera.reset();
    }
    
    // Smooth camera rotation
    float rotationSpeed = 1.0f; // Could use deltaTime if available, but processInput is called every frame
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        camera.rotateLeft(0.016f * rotationSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        camera.rotateRight(0.016f * rotationSpeed);
    }
}

void Application::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

std::string Application::getAreaFilePath(int selectedArea) const {
    switch (selectedArea) {
        case 0:  return "../data/moratuwa_area_real.json";
        case 1:  return "../data/borella_area_real.json";
        case 2:  return "../data/demo_traffic_area.json";
        case 3:  return "../data/random_city_area.json";
        default: return "../data/moratuwa_area_real.json";
    }
}