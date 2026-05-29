#pragma once

#include <string>
#include <vector>

struct Vec2 {
    float x;
    float y;

    Vec2(float x = 0.0f, float y = 0.0f)
        : x(x), y(y) {}
};

struct Road {
    std::string id;
    std::string name;
    std::vector<Vec2> points;
    int lanes = 1;
};

struct Building {
    std::string id;
    std::string name;
    std::vector<Vec2> base;
    float height = 50.0f;
};

struct TrafficLight {
    std::string id;
    Vec2 position;
    float redDuration = 20.0f;
    float yellowDuration = 5.0f;
    float greenDuration = 20.0f;
};

struct PedestrianCrossing {
    std::string id;
    std::vector<Vec2> points;
};

struct VehicleRoute {
    std::string id;
    std::vector<Vec2> points;
};

struct CityArea {
    std::string name;
    float latitude = 0.0f;
    float longitude = 0.0f;
    std::string defaultTrafficLevel;

    std::vector<Road> roads;
    std::vector<Building> buildings;
    std::vector<TrafficLight> trafficLights;
    std::vector<PedestrianCrossing> crossings;
    std::vector<VehicleRoute> routes;

    bool loaded = false;
};