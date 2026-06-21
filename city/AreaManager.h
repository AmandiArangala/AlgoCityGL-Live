/**
 * @file AreaManager.h
 * @brief Declares AreaManager — loads a city area from a JSON file.
 *
 * AreaManager is responsible for reading and parsing the project's JSON city-area
 * files (under data/) and producing a fully populated CityArea struct.
 *
 * JSON file structure (expected fields)
 * ──────────────────────────────────────
 * {
 *   "name": "...",
 *   "latitude": 0.0,
 *   "longitude": 0.0,
 *   "defaultTrafficLevel": "NORMAL",
 *   "roads":         [ { "id", "name", "lanes", "points": [[x,y], ...] } ],
 *   "buildings":     [ { "id", "name", "height", "base":   [[x,y], ...] } ],
 *   "trafficLights": [ { "id", "position": [x,y], "cycle": [red,yellow,green],
 *                        "initialState", "initialTimer", "direction": [dx,dy] } ],
 *   "crossings":     [ { "id", "points": [[x,y], ...] } ],
 *   "routes":        [ { "id", "points": [[x,y], ...] } ]
 * }
 *
 * Coordinate scaling
 * ───────────────────
 * All [x, y] arrays in the JSON are geographic offsets (small floats, e.g., 0–300).
 * parsePoint() multiplies them by SPREAD_FACTOR (3.0) to convert them to
 * screen-space pixel coordinates that fit the 1280×720 window.
 *
 * Dependencies
 * ─────────────
 * Uses the nlohmann/json single-header library (external/json/json.hpp).
 */

#pragma once

#include <string>
#include "CityArea.h"
#include "json.hpp" // nlohmann/json — single-header JSON parser.

class AreaManager {
public:
    /**
     * @brief Parse a city-area JSON file and store the result.
     *
     * Opens the file, parses JSON, populates a CityArea struct (roads,
     * buildings, traffic lights, crossings, routes), and stores it
     * internally.  Sets CityArea::loaded = true on success.
     *
     * @param filePath  Relative path to the JSON file (from the build/ directory).
     * @return true on success, false if the file cannot be opened or parsed.
     */
    bool loadAreaFromFile(const std::string& filePath);

    /**
     * @brief Return the most recently loaded city area (read-only).
     *
     * Only valid after a successful loadAreaFromFile() call.
     * Callers should check hasLoadedArea() first.
     */
    const CityArea& getCurrentArea() const;

    /**
     * @brief Return true if a city area has been successfully loaded.
     *
     * Returns false before the first successful loadAreaFromFile() call.
     */
    bool hasLoadedArea() const;

private:
    CityArea currentArea; ///< The most recently loaded city area (default-constructed until loaded).

    /**
     * @brief Parse a [x, y] JSON array into a Vec2, applying the SPREAD_FACTOR.
     *
     * All coordinates in the JSON files are scaled-down geographic offsets.
     * Multiplying by SPREAD_FACTOR (3.0) converts them to screen-space pixels
     * suitable for a 1280×720 display.
     *
     * @param pointJson  A JSON array with at least 2 numeric elements [x, y].
     * @return           The parsed Vec2 in screen-space pixels, or (0, 0) on error.
     */
    Vec2 parsePoint(const nlohmann::json& pointJson);
};