#include "AreaManager.h"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

Vec2 AreaManager::parsePoint(const json& pointJson) {
    if (!pointJson.is_array() || pointJson.size() < 2) {
        return Vec2(0.0f, 0.0f);
    }

    const float SPREAD_FACTOR = 3.0f;
    return Vec2(
        pointJson[0].get<float>() * SPREAD_FACTOR,
        pointJson[1].get<float>() * SPREAD_FACTOR
    );
}

bool AreaManager::loadAreaFromFile(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open area file: " << filePath << std::endl;
        return false;
    }

    json data;

    try {
        file >> data;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }

    CityArea area;

    area.name = data.value("name", "Unnamed Area");
    area.latitude = data.value("latitude", 0.0f);
    area.longitude = data.value("longitude", 0.0f);
    area.defaultTrafficLevel = data.value("defaultTrafficLevel", "NORMAL");

    if (data.contains("roads")) {
        for (const auto& roadJson : data["roads"]) {
            Road road;
            road.id = roadJson.value("id", "");
            road.name = roadJson.value("name", "");
            road.lanes = roadJson.value("lanes", 1);

            if (roadJson.contains("points")) {
                for (const auto& pointJson : roadJson["points"]) {
                    road.points.push_back(parsePoint(pointJson));
                }
            }

            area.roads.push_back(road);
        }
    }

    if (data.contains("buildings")) {
        for (const auto& buildingJson : data["buildings"]) {
            Building building;
            building.id = buildingJson.value("id", "");
            building.name = buildingJson.value("name", "");
            building.height = buildingJson.value("height", 50.0f) * 3.0f;

            if (buildingJson.contains("base")) {
                for (const auto& pointJson : buildingJson["base"]) {
                    building.base.push_back(parsePoint(pointJson));
                }
            }

            area.buildings.push_back(building);
        }
    }

    if (data.contains("trafficLights")) {
        for (const auto& signalJson : data["trafficLights"]) {
            TrafficLight light;
            light.id = signalJson.value("id", "");

            if (signalJson.contains("position")) {
                light.position = parsePoint(signalJson["position"]);
            }

            if (signalJson.contains("cycle") && signalJson["cycle"].is_array()) {
                const auto& cycle = signalJson["cycle"];

                if (cycle.size() >= 3) {
                    light.redDuration = cycle[0].get<float>();
                    light.yellowDuration = cycle[1].get<float>();
                    light.greenDuration = cycle[2].get<float>();
                }
            }

            light.initialState = signalJson.value("initialState", "Red");
            light.initialTimer = signalJson.value("initialTimer", 0.0f);
            
            if (signalJson.contains("direction")) {
                auto dirArr = signalJson["direction"];
                if (dirArr.is_array() && dirArr.size() >= 2) {
                    light.direction = Vec2(dirArr[0].get<float>(), dirArr[1].get<float>());
                }
            }

            area.trafficLights.push_back(light);
        }
    }

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

    area.loaded = true;
    currentArea = area;

    std::cout << "Loaded area: " << currentArea.name << std::endl;
    std::cout << "Roads: " << currentArea.roads.size() << std::endl;
    std::cout << "Buildings: " << currentArea.buildings.size() << std::endl;
    std::cout << "Traffic lights: " << currentArea.trafficLights.size() << std::endl;
    std::cout << "Routes: " << currentArea.routes.size() << std::endl;

    return true;
}

const CityArea& AreaManager::getCurrentArea() const {
    return currentArea;
}

bool AreaManager::hasLoadedArea() const {
    return currentArea.loaded;
}