/**
 * @file AreaManager.cpp
 * @brief Implements AreaManager — JSON parsing and city area construction.
 *
 * Parsing strategy
 * ─────────────────
 * The file uses nlohmann/json (a single-header C++ library) to deserialise
 * the JSON city-area files.  Each top-level array ("roads", "buildings", etc.)
 * is iterated and converted to the corresponding C++ struct.
 *
 * All coordinate arrays are passed through parsePoint() which applies the
 * SPREAD_FACTOR (3.0) scale so that geographic offsets become pixel distances.
 *
 * Defensive parsing
 * ──────────────────
 * - data.contains("key") is checked before accessing any optional field.
 * - value("key", default) provides safe defaults for missing scalar fields.
 * - An invalid point array falls back to Vec2(0, 0) instead of crashing.
 * - The entire JSON parse is wrapped in a try/catch so a malformed file
 *   prints an error and returns false rather than throwing to the caller.
 *
 * Building height scaling
 * ────────────────────────
 * Raw height values from the JSON are multiplied by 3.0 to give buildings
 * a visually proportionate extrusion in the 2.5D isometric view.
 */

#include "AreaManager.h"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// parsePoint — Convert a JSON [x, y] array to a scaled Vec2
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Parse a two-element JSON array into a Vec2 with the spread-factor applied.
 *
 * SPREAD_FACTOR = 3.0 scales the geographic coordinate offsets (which are small
 * numbers like 0–300) into screen-space pixel coordinates usable by the Renderer.
 *
 * @param pointJson  A JSON value that should be an array of at least 2 numbers.
 * @return           The scaled Vec2, or (0, 0) if the input is invalid.
 */
Vec2 AreaManager::parsePoint(const json& pointJson) {
    if (!pointJson.is_array() || pointJson.size() < 2) {
        return Vec2(0.0f, 0.0f); // Guard against malformed point arrays.
    }

    const float SPREAD_FACTOR = 3.0f; // Converts JSON units → screen pixels.
    return Vec2(
        pointJson[0].get<float>() * SPREAD_FACTOR,
        pointJson[1].get<float>() * SPREAD_FACTOR
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// loadAreaFromFile — Parse a JSON city-area file into a CityArea struct
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Open, parse, and store a city area from the given JSON file.
 *
 * Populates `currentArea` with roads, buildings, traffic lights, crossings,
 * and vehicle routes extracted from the file.  Sets currentArea.loaded = true
 * on success.
 *
 * @return true if the file was opened and parsed successfully.
 */
bool AreaManager::loadAreaFromFile(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open area file: " << filePath << std::endl;
        return false;
    }

    json data;

    try {
        file >> data; // Deserialise the entire JSON file.
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }

    CityArea area;

    // ── Top-level metadata ────────────────────────────────────────────────────
    area.name                = data.value("name", "Unnamed Area");
    area.latitude            = data.value("latitude", 0.0f);
    area.longitude           = data.value("longitude", 0.0f);
    area.defaultTrafficLevel = data.value("defaultTrafficLevel", "NORMAL");

    // ── Roads ─────────────────────────────────────────────────────────────────
    // Each road is a named polyline with a lane count.
    if (data.contains("roads")) {
        for (const auto& roadJson : data["roads"]) {
            Road road;
            road.id    = roadJson.value("id", "");
            road.name  = roadJson.value("name", "");
            road.lanes = roadJson.value("lanes", 1);

            if (roadJson.contains("points")) {
                for (const auto& pointJson : roadJson["points"]) {
                    road.points.push_back(parsePoint(pointJson));
                }
            }

            area.roads.push_back(road);
        }
    }

    // ── Buildings ─────────────────────────────────────────────────────────────
    // Buildings have a footprint polygon (base) and a height used for 2.5D extrusion.
    if (data.contains("buildings")) {
        for (const auto& buildingJson : data["buildings"]) {
            Building building;
            building.id   = buildingJson.value("id", "");
            building.name = buildingJson.value("name", "");
            // Scale height by 3.0 so buildings look proportionate in isometric view.
            building.height = buildingJson.value("height", 50.0f) * 3.0f;

            if (buildingJson.contains("base")) {
                for (const auto& pointJson : buildingJson["base"]) {
                    building.base.push_back(parsePoint(pointJson));
                }
            }

            area.buildings.push_back(building);
        }
    }

    // ── Traffic Lights ────────────────────────────────────────────────────────
    // Each traffic light has a position, a [red, yellow, green] cycle, an
    // initial state, and an optional direction vector for lane-specific control.
    if (data.contains("trafficLights")) {
        for (const auto& signalJson : data["trafficLights"]) {
            TrafficLight light;
            light.id = signalJson.value("id", "");

            if (signalJson.contains("position")) {
                light.position = parsePoint(signalJson["position"]);
            }

            // The "cycle" array is [redDuration, yellowDuration, greenDuration] in seconds.
            if (signalJson.contains("cycle") && signalJson["cycle"].is_array()) {
                const auto& cycle = signalJson["cycle"];

                if (cycle.size() >= 3) {
                    light.redDuration    = cycle[0].get<float>();
                    light.yellowDuration = cycle[1].get<float>();
                    light.greenDuration  = cycle[2].get<float>();
                }
            }

            light.initialState = signalJson.value("initialState", "Red");
            light.initialTimer = signalJson.value("initialTimer", 0.0f);

            // Optional direction vector: allows lane-specific signal control.
            // A zero vector means the light governs all approaching vehicles.
            if (signalJson.contains("direction")) {
                auto dirArr = signalJson["direction"];
                if (dirArr.is_array() && dirArr.size() >= 2) {
                    light.direction = Vec2(dirArr[0].get<float>(), dirArr[1].get<float>());
                }
            }

            area.trafficLights.push_back(light);
        }
    }

    // ── Pedestrian Crossings ──────────────────────────────────────────────────
    // Crossings are polygon regions drawn as striped zebra markings.
    if (data.contains("crossings")) {
        for (const auto& crossingJson : data["crossings"]) {
            PedestrianCrossing crossing;
            crossing.id = crossingJson.value("id", "");

            if (crossingJson.contains("points")) {
                for (const auto& pointJson : crossingJson["points"]) {
                    crossing.points.push_back(parsePoint(pointJson));
                }
            }

            area.crossings.push_back(crossing);
        }
    }

    // ── Vehicle Routes ────────────────────────────────────────────────────────
    // Each route is an ordered list of waypoints that one or more vehicles follow.
    if (data.contains("routes")) {
        for (const auto& routeJson : data["routes"]) {
            VehicleRoute route;
            route.id = routeJson.value("id", "");

            if (routeJson.contains("points")) {
                for (const auto& pointJson : routeJson["points"]) {
                    route.points.push_back(parsePoint(pointJson));
                }
            }

            area.routes.push_back(route);
        }
    }

    // Mark the area as successfully loaded and replace the previous area.
    area.loaded = true;
    currentArea = area;

    // Print a summary to the console for debugging during development.
    std::cout << "Loaded area: "        << currentArea.name                   << std::endl;
    std::cout << "Roads: "              << currentArea.roads.size()           << std::endl;
    std::cout << "Buildings: "          << currentArea.buildings.size()       << std::endl;
    std::cout << "Traffic lights: "     << currentArea.trafficLights.size()   << std::endl;
    std::cout << "Routes: "             << currentArea.routes.size()          << std::endl;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Return the most recently loaded city area (read-only). */
const CityArea& AreaManager::getCurrentArea() const {
    return currentArea;
}

/** @brief Return true if currentArea.loaded is set (i.e., a file was parsed). */
bool AreaManager::hasLoadedArea() const {
    return currentArea.loaded;
}