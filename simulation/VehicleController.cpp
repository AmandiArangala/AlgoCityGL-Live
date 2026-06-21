/**
 * @file VehicleController.cpp
 * @brief Implements vehicle spawning and per-frame updates.
 *
 * Vehicle spawning strategy:
 *   For each VehicleRoute with N >= 2 waypoints:
 *     - numVehicles = N / 5  (one vehicle per 5 waypoints, minimum 1).
 *     - Each vehicle is pre-advanced by v * 8 seconds so they start
 *       evenly spaced along the route rather than all bunched at the start.
 *
 * Dense-map throttling:
 *   If the area has > 150 routes (e.g. Borella, Pettah), only every 4th
 *   route spawns vehicles to keep the frame rate manageable.
 */
#include "VehicleController.h"
#include <iostream>

void VehicleController::initializeFromArea(const CityArea& area) {
    vehicles.clear();
    allRoutes = area.routes; // Store all routes for junction routing during simulation.

    std::cout << "Initializing vehicles from routes..." << std::endl;

    // For very dense real-world maps, cap vehicle density to ~25% to maintain performance.
    int skipStep = 1;
    if (area.routes.size() > 150) {
        skipStep = 4; // Capping vehicle density to ~25% for dense real-world maps (e.g. Borella, Pettah)
    }

    int index = 0;
    for (const VehicleRoute& route : area.routes) {
        // Apply the skip step: only process every Nth route.
        if (index % skipStep != 0) {
            index++;
            continue;
        }
        index++;

        std::cout << "Checking route: " << route.id
                  << " with " << route.points.size()
                  << " points" << std::endl;

        if (route.points.size() >= 2) {
            // Spawn one vehicle per 5 waypoints (at least 1).
            int numVehicles = route.points.size() / 5;
            if (numVehicles < 1) numVehicles = 1;

            for (int v = 0; v < numVehicles; v++) {
                Vehicle vehicle;
                vehicle.setRoute(route.points);

                // Pre-advance the vehicle by v*8 seconds so vehicles on the same
                // route start evenly distributed rather than all at waypoint 0.
                std::vector<RuntimeTrafficLight> dummyLights;
                float advanceTime = v * 8.0f; // Space them by 8 seconds of driving
                for (float t = 0; t < advanceTime; t += 0.1f) {
                    std::vector<Vehicle> emptyVehicles;
                    vehicle.update(0.1f, dummyLights, emptyVehicles, allRoutes);
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
    // Do nothing while paused — vehicles hold their positions.
    if (!isPlaying) {
        return;
    }

    // Guard against absurdly large or zero time steps that could cause vehicles
    // to teleport across the map or stand still unexpectedly.
    if (deltaTime <= 0.0f || deltaTime > 1.0f) {
        deltaTime = 1.0f / 60.0f;
    }

    // Update every vehicle, passing all other vehicles for collision avoidance.
    for (Vehicle& vehicle : vehicles) {
        vehicle.update(deltaTime, trafficLights, vehicles, allRoutes);
    }
}

void VehicleController::reset() {
    // Reset each vehicle independently back to its route starting position.
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