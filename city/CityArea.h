/**
 * @file CityArea.h
 * @brief Plain data structures that represent a loaded city map.
 *
 * These POD (Plain Old Data) structs are populated by AreaManager when it
 * parses a JSON city-area file.  They are then consumed by the Renderer,
 * VehicleController, and SignalController.
 *
 * Coordinate system:
 *   - All Vec2 positions are in "world units" — raw JSON values scaled by 3.
 *   - Screen coordinates are computed by Camera2D::worldToScreen().
 */
#pragma once

#include <string>
#include <vector>

/**
 * @brief A 2D floating-point point / vector used throughout the project.
 */
struct Vec2 {
    float x; ///< Horizontal component.
    float y; ///< Vertical component (increases downward on screen).

    /// Default constructor initialises both components to zero.
    Vec2(float x = 0.0f, float y = 0.0f)
        : x(x), y(y) {}
};

/**
 * @brief A single road segment made up of a polyline of Vec2 waypoints.
 */
struct Road {
    std::string id;             ///< Unique identifier from the JSON file.
    std::string name;           ///< Human-readable road name (shown as label).
    std::vector<Vec2> points;   ///< Ordered list of waypoints along the road centreline.
    int lanes = 1;              ///< Number of lanes (1, 2, or 3) — affects road width.
};

/**
 * @brief A building defined by a ground-floor polygon (base) and a height.
 */
struct Building {
    std::string id;           ///< Unique identifier from the JSON file.
    std::string name;         ///< Display name (shown as building label).
    std::vector<Vec2> base;   ///< Polygon vertices of the building footprint.
    float height = 50.0f;     ///< Building height in world units (scaled \u00d73 from JSON).
};

/**
 * @brief Static data for a traffic light, as read from the JSON file.
 *
 * The runtime state (current phase, countdown timer) is stored in
 * RuntimeTrafficLight inside SignalController.
 */
struct TrafficLight {
    std::string id;                       ///< Unique identifier.
    Vec2 position;                        ///< World position of the light.
    float redDuration    = 20.0f;         ///< Seconds the light stays Red.
    float yellowDuration =  5.0f;         ///< Seconds the light stays Yellow.
    float greenDuration  = 20.0f;         ///< Seconds the light stays Green.
    std::string initialState = "Red";     ///< Starting phase ("Red", "Yellow", or "Green").
    float initialTimer   = 0.0f;          ///< Elapsed time within the initial phase at load time.
    Vec2 direction = Vec2(0.0f, 0.0f);   ///< Unit vector of the road direction this light governs.
};

/** @brief A zebra / pedestrian crossing drawn as a striped overlay on roads. */
struct PedestrianCrossing {
    std::string id;           ///< Unique identifier.
    std::vector<Vec2> points; ///< Corner points defining the crossing polygon.
};

/**
 * @brief An ordered sequence of waypoints that vehicles follow.
 *
 * VehicleController creates one or more Vehicle objects per route, spacing
 * them out along the waypoint list so they do not overlap at startup.
 */
struct VehicleRoute {
    std::string id;           ///< Unique route identifier (e.g. "route_1").
    std::vector<Vec2> points; ///< Ordered waypoints from start to end.
};

/**
 * @brief Top-level container for everything that belongs to one city map.
 *
 * A CityArea is created empty, then populated by AreaManager::loadAreaFromFile().
 * The \`loaded\` flag is set to true only after a successful parse, so other
 * systems can safely check \`hasLoadedArea()\` before reading the data.
 */
struct CityArea {
    std::string name;                            ///< Human-readable area name (e.g. \"University of Moratuwa\").
    float latitude  = 0.0f;                      ///< Latitude of the map centre (informational).
    float longitude = 0.0f;                      ///< Longitude of the map centre (informational).
    std::string defaultTrafficLevel;             ///< Default congestion level string from JSON.

    std::vector<Road>              roads;        ///< All road segments in this area.
    std::vector<Building>          buildings;    ///< All buildings in this area.
    std::vector<TrafficLight>      trafficLights;///< Static traffic light data.
    std::vector<PedestrianCrossing> crossings;   ///< Pedestrian crossing polygons.
    std::vector<VehicleRoute>      routes;       ///< Vehicle navigation routes.

    bool loaded = false; ///< True only after a successful JSON parse.
};