#pragma once

#include <string>

#include "Renderer.h"
#include "ImGuiPanels.h"
#include "AreaManager.h"
#include "VehicleController.h"

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

    bool initialize();
    void shutdown();
    void processInput();

    std::string getAreaFilePath(int selectedArea) const;
};