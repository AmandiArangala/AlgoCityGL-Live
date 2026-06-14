#include "VehicleController.h"
#include <iostream>

void VehicleController::initializeFromArea(const CityArea& area) {
    vehicles.clear();

    std::cout << "Initializing vehicles from routes..." << std::endl;

    int skipStep = 1;
    if (area.routes.size() > 150) {
        skipStep = 4; // Capping vehicle density to ~25% for dense real-world maps (e.g. Borella, Pettah)
    }

    int index = 0;
    for (const VehicleRoute& route : area.routes) {
        if (index % skipStep != 0) {
            index++;
            continue;
        }
        index++;

        std::cout << "Checking route: " << route.id
                  << " with " << route.points.size()
                  << " points" << std::endl;

        if (route.points.size() >= 2) {
            int numVehicles = route.points.size() / 5;
            if (numVehicles < 1) numVehicles = 1;

            for (int v = 0; v < numVehicles; v++) {
                Vehicle vehicle;
                vehicle.setRoute(route.points);
                
                std::vector<RuntimeTrafficLight> dummyLights;
                float advanceTime = v * 8.0f; // Space them by 8 seconds of driving
                for (float t = 0; t < advanceTime; t += 0.1f) {
                    std::vector<Vehicle> emptyVehicles;
                    vehicle.update(0.1f, dummyLights, emptyVehicles);
                }

                vehicles.push_back(vehicle);
            }

            std::cout << numVehicles << " vehicles added for route: " << route.id << std::endl;
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
        vehicle.update(deltaTime, trafficLights, vehicles);
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