# AlgoCityGL Live

## Area-Based 2.5D Smart Traffic Graphics Simulator

AlgoCityGL Live is an OpenGL-based Computer Graphics project that loads real or real-city-inspired area layouts and simulates smart traffic on top of them. The project is designed for a university Computer Graphics module and demonstrates important graphics algorithms through an interactive 2.5D smart city traffic environment.

The user can select a city area such as Pettah / Colombo Fort, Borella Junction, or University of Moratuwa area. After selecting an area, the system loads a city layout with roads, buildings, traffic lights, pedestrian crossings, vehicle routes, and traffic density information. Vehicles then move along the roads, stop at red lights, move at green lights, turn at junctions, and react to traffic density, weather, and time-of-day modes.

The city is displayed using a 2.5D isometric effect, so it looks like a 3D city while still being mainly built using 2D computer graphics algorithms.

---

## Main Objective

The main objective of this project is to combine a practical smart traffic simulation with core Computer Graphics theory.

This is not just a traffic animation. The project uses traffic simulation as a visual environment to demonstrate how graphics algorithms are used to draw, transform, fill, clip, and view 2D/2.5D objects.

---

## Key Features

- Area-based city selection
- Real or real-city-inspired road/building layouts
- 2.5D isometric city rendering
- Moving vehicles on predefined routes
- Traffic lights with red, yellow, and green states
- Vehicles stop at red lights and move at green lights
- Traffic density modes
- Weather and time-of-day modes
- Mini-map view
- Zoom and pan controls
- Algorithm X-Ray Mode
- Theory dashboard for Computer Graphics concepts
- Manual implementation of important graphics algorithms

---

## Area Selection

The system supports selecting different city areas, such as:

- University of Moratuwa area
- Pettah / Colombo Fort area
- Borella Junction
- Custom demo area

Each area can contain:

- Roads
- Buildings
- Traffic lights
- Pedestrian crossings
- Vehicle routes
- Default traffic density
- Building heights
- Simulation rules

---

## Main Unique Feature

### Algorithm X-Ray Mode

In normal mode, the user sees a clean 2.5D smart city traffic simulation.

In Algorithm X-Ray Mode, the system reveals the graphics algorithms behind the simulation, such as:

- DDA line pixels
- Bresenham line pixels
- Midpoint circle points
- Scan-line filling lines
- Vehicle transformation matrices
- Clipping windows
- Viewport mapping

This makes the project strongly connected to Computer Graphics theory.

---

## Computer Graphics Concepts Used

### 1. 2D Drawing and Rasterization

- Pixels
- Rasterization
- Scan conversion
- Output primitives
- Lines
- Circles
- Polygons

Algorithms:

- DDA line drawing algorithm
- Bresenham line drawing algorithm
- Midpoint circle algorithm
- Bresenham circle algorithm

Used for:

- Road edges
- Lane markings
- Vehicle wheels
- Traffic light bulbs
- Road signs
- Building outlines

---

### 2. 2D Transformations

- Translation
- Rotation
- Scaling
- Reflection
- Shearing
- Matrix representation
- Homogeneous coordinates
- Composite transformations
- Inverse transformations

Used for:

- Vehicle movement
- Vehicle turning
- Object scaling
- Shadow effects
- Camera movement
- Transformation matrix visualization

---

### 3. Polygon Filling and Region Filling

- Scan-line polygon filling
- Odd-even rule
- Intersection sorting
- Vertex handling
- Horizontal edge handling
- Flood fill
- Boundary fill
- 4-connected fill
- 8-connected fill

Used for:

- Buildings
- Roads
- Parks
- Vehicle bodies
- Traffic zones
- Heat maps

---

### 4. Viewing and Clipping

- World coordinate system
- Screen coordinate system
- World window
- Viewport
- Window-to-viewport mapping
- Zooming
- Panning
- Mini-map

Algorithms:

- Point clipping
- Cohen-Sutherland line clipping
- Liang-Barsky line clipping
- Sutherland-Hodgman polygon clipping

---

### 5. 2.5D Graphics

- Isometric projection
- Building extrusion
- Depth sorting
- Layered rendering
- Shadows
- Day/night visual effects
- Vehicle headlights

Used to make the city look like 3D while still using 2D algorithms.

---

## Smart Traffic Simulation Concepts

The project includes traffic simulation features such as:

- Vehicle route following
- Vehicle speed control
- Traffic signal timing
- Red/yellow/green signal states
- Stop/go behavior
- Traffic density
- Adaptive traffic signal idea
- Pedestrian crossings
- Incident mode
- Weather effect on vehicle speed

---

## Live Context Modes

The system can support live or simulated context modes:

- Normal
- Rain
- Night
- Heavy Traffic
- Incident

Example effects:

| Mode | Simulation Effect |
|------|------------------|
| Normal | Normal speed and lighting |
| Rain | Slower vehicles, wet roads, rain effect |
| Night | Headlights, streetlights, darker scene |
| Heavy Traffic | More vehicles, slower movement, heat map |
| Incident | One road or lane slows down / becomes blocked |

Optional API support can be added for live weather using OpenWeather API.

---

## Technologies Used

- C++
- OpenGL
- GLFW
- GLAD
- Dear ImGui
- GLM
- CMake
- JSON data files
- VS Code
- GitHub

Optional:

- OpenWeather API
- OpenStreetMap / Overpass Turbo
- nlohmann/json
- Postman for API testing

---

## Development Tools

Recommended tools:

- Visual Studio Code
- CMake Tools extension
- C/C++ extension
- Git
- GitHub
- CMake
- MinGW-w64 / MSYS2 / Visual Studio Build Tools
- Overpass Turbo for map layout extraction
- Postman for API testing

---

## Project Folder Structure

```text
AlgoCityGL-Live/
│
├── CMakeLists.txt
├── README.md
├── main.cpp
│
├── include/
│   ├── Application.h
│   ├── Renderer.h
│   ├── Camera2D.h
│   ├── Color.h
│   ├── Vec2.h
│   └── Config.h
│
├── src/
│   ├── Application.cpp
│   ├── Renderer.cpp
│   ├── Camera2D.cpp
│   └── Config.cpp
│
├── graphics/
│   ├── PixelBuffer.h
│   ├── PixelBuffer.cpp
│   ├── Rasterizer.h
│   ├── Rasterizer.cpp
│   ├── LineAlgorithms.h
│   ├── LineAlgorithms.cpp
│   ├── CircleAlgorithms.h
│   ├── CircleAlgorithms.cpp
│   ├── FillAlgorithms.h
│   ├── FillAlgorithms.cpp
│   ├── Matrix3x3.h
│   ├── Matrix3x3.cpp
│   ├── ClippingAlgorithms.h
│   ├── ClippingAlgorithms.cpp
│   ├── Projection2_5D.h
│   └── Projection2_5D.cpp
│
├── city/
│   ├── AreaManager.h
│   ├── AreaManager.cpp
│   ├── CityArea.h
│   ├── Road.h
│   ├── Building.h
│   ├── Vehicle.h
│   ├── Vehicle.cpp
│   ├── TrafficLight.h
│   ├── TrafficLight.cpp
│   ├── PedestrianCrossing.h
│   ├── RouteManager.h
│   └── MiniMap.h
│
├── simulation/
│   ├── TrafficSystem.h
│   ├── TrafficSystem.cpp
│   ├── SignalController.h
│   ├── SignalController.cpp
│   ├── VehicleController.h
│   └── VehicleController.cpp
│
├── live/
│   ├── LiveContextEngine.h
│   ├── LiveContextEngine.cpp
│   ├── WeatherClient.h
│   ├── WeatherClient.cpp
│   ├── MockLiveData.h
│   └── MockLiveData.cpp
│
├── ui/
│   ├── ImGuiPanels.h
│   ├── ImGuiPanels.cpp
│   ├── TheoryDashboard.h
│   └── TheoryDashboard.cpp
│
├── data/
│   ├── pettah_area.json
│   ├── borella_area.json
│   └── moratuwa_area.json
│
└── external/
    ├── glad/
    ├── imgui/
    └── json/
