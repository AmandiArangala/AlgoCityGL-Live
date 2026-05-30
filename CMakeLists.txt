cmake_minimum_required(VERSION 3.16)

project(AlgoCityGL_Live)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLAD_DIR ${CMAKE_SOURCE_DIR}/external/glad)
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/external/glfw)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_executable(AlgoCityGL_Live
    main.cpp

    src/Application.cpp
    src/Renderer.cpp
    src/ImGuiPanels.cpp
    src/Camera2D.cpp

    graphics/PixelBuffer.cpp
    graphics/LineAlgorithms.cpp
    graphics/CircleAlgorithms.cpp
    graphics/Projection2_5D.cpp
    graphics/Matrix3x3.cpp
    graphics/FillAlgorithms.cpp

    city/AreaManager.cpp
    city/Vehicle.cpp

    live/LiveContextEngine.cpp

    simulation/VehicleController.cpp
    simulation/SignalController.cpp

    ${GLAD_DIR}/src/glad.c

    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(AlgoCityGL_Live PRIVATE
    include
    graphics
    city
    simulation
    live
    external/json
    ${GLAD_DIR}/include
    ${GLFW_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(AlgoCityGL_Live PRIVATE
    ${GLFW_DIR}/lib/libglfw3.a
    opengl32
    gdi32
    user32
    shell32
)