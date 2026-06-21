/**
 * @file CityArea.h
 * @brief Plain data structures that represent a loaded city map.
 *
 * These POD (Plain Old Data) structs are populated by AreaManager when it parses
 * a JSON city-area file.  They are then read by the Renderer, VehicleController,
 * and SignalController every frame.
 *
 * Coordinate system
 * -----------------
 * Raw JSON coordinates are geographic offsets (small floating-point numbers).
 * AreaManager multiplies them by SPREAD_FACTOR (3.0) to convert them to screen-space
 * pixel coordinates before storing them in these structs.
 */

#pragma once

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Vec2 — 2D floating-point vector used for all positions and directions.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A 2D point or direction vector (x, y).
 *
 * Used everywhere: road waypoints, building corners, vehicle positions,
 * traffic-light positions, camera offsets, etc.
 */
struct Vec2 {
    float x;   ///< Horizontal screen coordinate (or direction component).
    float y;   ///< Vertical screen coordinate (or direction component).

    /** @brief Construct a Vec2; defaults to the origin (0, 0). */
    Vec2(float x = 0.0f, float y = 0.0f)
        : x(x), y(y) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Road — a polyline that vehicles drive along.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A road segment consisting of an ordered list of waypoints.
 *
 * Roads are rendered as coloured polylines. The `lanes` field controls the
 * visual width drawn by the Renderer.
 */
struct Road {
    std::string       id;     ///< Unique road identifier (from JSON).
    std::string       name;   ///< Human-readable road name (e.g., "Galle Road").
    std::vector<Vec2> points; ///< Ordered waypoints defining the road shape.
    int               lanes = 1; ///< Number of lanes (affects rendered width).
};

// ─────────────────────────────────────────────────────────────────────────────
// Building — a polygon footprint with an extruded height for 2.5D rendering.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A city building with a ground-floor polygon and a height value.
 *
 * In 2.5D (isometric) mode, the Renderer uses the `base` polygon as the
 * roof outline and shifts it upward by `height` to draw the visible face.
 */
struct Building {
    std::string       id;           ///< Unique building identifier.
    std::string       name;         ///< Display name (shown as a label on screen).
    std::vector<Vec2> base;         ///< Ordered corner points of the ground footprint.
    float             height = 50.0f; ///< Visual extrusion height in screen units (pre-scaled).
};

// ─────────────────────────────────────────────────────────────────────────────
// TrafficLight — static data for a single signal head.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Static configuration for one traffic-light signal.
 *
 * The SignalController wraps this in a RuntimeTrafficLight to add live
 * state (current colour) and a running timer.
 *
 * The optional `direction` vector lets a light govern only vehicles moving
 * in a roughly matching direction (dot-product > 0.8 check in Vehicle).
 * A zero direction vector means the light governs all approaching vehicles.
 */
struct TrafficLight {
    std::string id;                         ///< Unique signal identifier.
    Vec2        position;                   ///< World-space position of the signal head.
    float       redDuration    = 20.0f;     ///< Seconds the signal stays Red.
    float       yellowDuration =  5.0f;     ///< Seconds the signal stays Yellow.
    float       greenDuration  = 20.0f;     ///< Seconds the signal stays Green.
    std::string initialState   = "Red";     ///< Starting colour ("Red", "Yellow", "Green").
    float       initialTimer   =  0.0f;     ///< Elapsed time within that initial state.
    Vec2        direction = Vec2(0.0f, 0.0f); ///< Unit vector for directional governance (zero = all).
};

// ─────────────────────────────────────────────────────────────────────────────
// PedestrianCrossing — a zebra crossing polygon.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Marks a pedestrian crossing area with a list of boundary points.
 *
 * Rendered as a striped rectangle across the road.
 */
struct PedestrianCrossing {
    std::string       id;     ///< Unique crossing identifier.
    std::vector<Vec2> points; ///< Corner points defining the crossing polygon.
};

// ─────────────────────────────────────────────────────────────────────────────
// VehicleRoute — a predefined path a vehicle follows.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief An ordered list of waypoints that a vehicle loops along.
 *
 * VehicleController creates one or more vehicles per route and staggers
 * their starting positions so the road feels populated.
 */
struct VehicleRoute {
    std::string       id;     ///< Unique route identifier.
    std::vector<Vec2> points; ///< Ordered waypoints the vehicle follows in sequence.
};

// ─────────────────────────────────────────────────────────────────────────────
// CityArea — the top-level container for one loaded map.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Aggregates all data for a single city area.
 *
 * Populated by AreaManager::loadAreaFromFile().  The `loaded` flag is false
 * until a successful parse, allowing the Application to show the default
 * test scene while no area is loaded.
 */
struct CityArea {
    std::string name;                ///< Human-readable area name (e.g., "University of Moratuwa").
    float       latitude  = 0.0f;   ///< Geographic latitude (informational only).
    float       longitude = 0.0f;   ///< Geographic longitude (informational only).
    std::string defaultTrafficLevel; ///< Baseline traffic density string from JSON.

    std::vector<Road>               roads;         ///< All roads in this area.
    std::vector<Building>           buildings;     ///< All buildings in this area.
    std::vector<TrafficLight>       trafficLights; ///< All static traffic-light configs.
    std::vector<PedestrianCrossing> crossings;     ///< All pedestrian crossings.
    std::vector<VehicleRoute>       routes;        ///< All vehicle routes.

    bool loaded = false; ///< True only after a successful loadAreaFromFile() call.
};