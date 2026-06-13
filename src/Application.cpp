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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, width, height);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
    });

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
    while (!glfwWindowShouldClose(window)) {
        processInput();

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        imguiPanels.render();
        liveContextEngine.setMode(imguiPanels.getSelectedWeatherMode());

        if (imguiPanels.consumeLoadAreaRequest()) {
            std::string filePath = getAreaFilePath(imguiPanels.getSelectedArea());

            if (areaManager.loadAreaFromFile(filePath)) {
                vehicleController.initializeFromArea(areaManager.getCurrentArea());
                signalController.initializeFromArea(areaManager.getCurrentArea());
            }
        }

        if (imguiPanels.consumeResetRequest()) {
            vehicleController.reset();
            signalController.reset();
        }

        if (imguiPanels.getIsPlaying()) {
            signalController.update(1.0f / 60.0f);
        }

        vehicleController.update(
            (1.0f / 60.0f) * liveContextEngine.getVehicleSpeedMultiplier(),
            imguiPanels.getIsPlaying(),
            signalController.getTrafficLights()
        );

        renderer.renderBackground();

        if (areaManager.hasLoadedArea()) {
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
            renderer.renderDay2TestScene(
                imguiPanels.getXRayMode(),
                imguiPanels.getSelectedLineAlgorithm()
            );
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
        default: return "../data/moratuwa_area_real.json";
    }
}