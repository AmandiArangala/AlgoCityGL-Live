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
    Application(int width, int height, const std::string& title);
    ~Application();

    void run();

private:
    int width;
    int height;
    std::string title;

    GLFWwindow* window;

    Renderer renderer;
    ImGuiPanels imguiPanels;
    AreaManager areaManager;
    VehicleController vehicleController;
    SignalController signalController;
    Camera2D camera;
    LiveContextEngine liveContextEngine;

    bool initialize();
    void shutdown();
    void processInput();

    std::string getAreaFilePath(int selectedArea) const;
};