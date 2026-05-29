#include "VehicleController.h"

void VehicleController::initializeFromArea(const CityArea& area) {
    vehicles.clear();

    for (const VehicleRoute& route : area.routes) {
        if (route.points.size() >= 2) {
            Vehicle vehicle;
            vehicle.setRoute(route.points);
            vehicles.push_back(vehicle);
        }
    }
}

void VehicleController::update(float deltaTime, bool isPlaying) {
    if (!isPlaying) {
        return;
    }

    for (Vehicle& vehicle : vehicles) {
        vehicle.update(deltaTime);
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