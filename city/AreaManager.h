/**
 * @file AreaManager.h
 * @brief Declares AreaManager — loads a city area from a JSON file into a CityArea struct.
 *
 * AreaManager is responsible for reading the project's JSON city-area files
 * (under data/) and producing a fully-populated CityArea.  It uses the
 * nlohmann/json single-header library for parsing.
 *
 * Usage pattern (driven by Application):
 *   areaManager.loadAreaFromFile(filePath);   // parse JSON
 *   const CityArea& area = areaManager.getCurrentArea(); // use data
 */
#pragma once

#include <string>
#include "CityArea.h"
#include "json.hpp"

class AreaManager {
public:
    /**
     * @brief Parses a JSON file and stores the result in currentArea.
     * @param filePath  Path to the JSON area file (relative to CWD).
     * @return true on success; false if the file cannot be opened or parsed.
     */
    bool loadAreaFromFile(const std::string& filePath);

    /** @brief Returns the most recently loaded city area. */
    const CityArea& getCurrentArea() const;

    /** @brief Returns true if a city area has been successfully loaded. */
    bool hasLoadedArea() const;

private:
    CityArea currentArea; ///< Stores the last successfully parsed area.

    /**
     * @brief Converts a JSON [x, y] array into a Vec2, scaled by SPREAD_FACTOR.
     * @param pointJson  A JSON array with at least 2 numeric elements.
     * @return           Scaled Vec2, or (0,0) if the array is malformed.
     */
    Vec2 parsePoint(const nlohmann::json& pointJson);
};