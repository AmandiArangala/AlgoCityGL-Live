#pragma once

#include <string>
#include "CityArea.h"
#include "json.hpp"

class AreaManager {
public:
    bool loadAreaFromFile(const std::string& filePath);

    const CityArea& getCurrentArea() const;
    bool hasLoadedArea() const;

private:
    CityArea currentArea;

    Vec2 parsePoint(const nlohmann::json& pointJson);
};