#include "VehicleController.h"
#include <iostream>

void VehicleController::initializeFromArea(const CityArea& area) {
    vehicles.clear();

    std::cout << "Initializing vehicles from routes..." << std::endl;

    for (const VehicleRoute& route : area.routes) {
        std::cout << "Checking route: " << route.id
                  << " with " << route.points.size()
                  << " points" << std::endl;

        if (route.points.size() >= 2) {
            Vehicle vehicle;
            vehicle.setRoute(route.points);
            vehicles.push_back(vehicle);

            std::cout << "Vehicle added for route: " << route.id << std::endl;
        }
    }

    std::cout << "Total vehicles initialized: " << vehicles.size() << std::endl;
}

void VehicleController::update(
    float deltaTime,
    bool isPlaying,
    const std::vector<RuntimeTrafficLight>& trafficLights
) {
    if (!isPlaying) {
        return;
    }

    if (deltaTime <= 0.0f || deltaTime > 1.0f) {
        deltaTime = 1.0f / 60.0f;
    }

    for (Vehicle& vehicle : vehicles) {
        vehicle.update(deltaTime, trafficLights);
    }
}

void VehicleController::reset() {
    for (Vehicle& vehicle : vehicles) {
        vehicle.reset();
    }
}

const std::vector<Vehicle>& VehicleController::getVehicles() const {
    return vehicles;
}

bool VehicleController::hasVehicles() const {
    return !vehicles.empty();
}