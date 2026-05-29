#pragma once

#include <string>

#include "Renderer.h"
#include "ImGuiPanels.h"

// Forward declaration instead of including GLFW here
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

    bool initialize();
    void shutdown();
    void processInput();
};