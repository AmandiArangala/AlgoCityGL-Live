/**
 * @file VehicleController.cpp
 * @brief Implements vehicle spawning and per-frame updates.
 *
 * Vehicle spawning strategy
 * ──────────────────────────
 * For each VehicleRoute with N ≥ 2 waypoints:
 *   numVehicles = max(1, N / 5)
 *
 * Each subsequent vehicle is "pre-simulated" for (v × 8) seconds with no
 * traffic lights, so they start at evenly spaced positions along the route.
 * This avoids the jarring "all cars depart from the same point at t=0" artifact.
 *
 * Density cap
 * ────────────
 * Real-world areas (Borella, Pettah) can have 200+ routes.  Spawning a vehicle
 * for every route would overload the CPU.  When routes > 150, only every 4th
 * route (25%) gets vehicles.  `skipStep` implements this modulo selection.
 *
 * allRoutes reference
 * ────────────────────
 * Vehicle::update() receives allRoutes so it can dynamically switch lanes at
 * junctions.  When a vehicle nears the end of its current route segment, it
 * searches allRoutes for a compatible connecting route and switches to it,
 * creating emergent turn behaviour at intersections.
 */

#include "VehicleController.h"
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// initializeFromArea — Spawn vehicles for the loaded city area
// ─────────────────────────────────────────────────────────────────────────────

void VehicleController::initializeFromArea(const CityArea& area) {
    vehicles.clear();           // Remove any vehicles from a previous area.
    allRoutes = area.routes;    // Cache all routes for dynamic lane-switching.

    std::cout << "Initializing vehicles from routes..." << std::endl;

    // ── Density control ───────────────────────────────────────────────────────
    // For very large real-world maps (e.g., Borella or Pettah), limit the number
    // of active routes to avoid performance degradation.
    int skipStep = 1; // By default, use every route.
    if (area.routes.size() > 150) {
        skipStep = 4; // Capping vehicle density to ~25% for dense real-world maps (e.g. Borella, Pettah)
    }

    int index = 0;
    for (const VehicleRoute& route : area.routes) {
        // Skip routes according to the density cap (modulo skipStep).
        if (index % skipStep != 0) {
            index++;
            continue;
        }
        index++;

        std::cout << "Checking route: " << route.id
                  << " with " << route.points.size()
                  << " points" << std::endl;

        if (route.points.size() >= 2) {
            // Spawn one vehicle per 5 waypoints on this route (minimum 1).
            // Longer routes get more vehicles so they look equally populated.
            int numVehicles = route.points.size() / 5;
            if (numVehicles < 1) numVehicles = 1;

            for (int v = 0; v < numVehicles; v++) {
                Vehicle vehicle;
                vehicle.setRoute(route.points); // Assign this route to the vehicle.

                // ── Pre-simulate to stagger starting positions ────────────────
                // Advance vehicle v by (v × 8) seconds with no traffic lights.
                // This distributes vehicles evenly along the route at spawn time,
                // creating a natural-looking pre-populated road.
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

// ─────────────────────────────────────────────────────────────────────────────
// update — Advance all vehicles one simulation step
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Update all vehicles by deltaTime seconds.
 *
 * Each vehicle receives the full vehicle list (for collision avoidance) and
 * all routes (for dynamic lane-switching at junctions).
 *
 * The deltaTime sanity clamp [0, 1s] prevents vehicles from teleporting if
 * the application hangs for more than one second (e.g., during file I/O).
 */
void VehicleController::update(
    float deltaTime,
    bool isPlaying,
    const std::vector<RuntimeTrafficLight>& trafficLights
) {
    // Do nothing while paused — vehicles hold their positions.
    if (!isPlaying) {
        return; // Simulation is paused; do not advance any vehicles.
    }

    // Clamp deltaTime to prevent large position jumps on slow frames.
    if (deltaTime <= 0.0f || deltaTime > 1.0f) {
        deltaTime = 1.0f / 60.0f; // Fall back to 60 FPS step if value is unreasonable.
    }

    // Update each vehicle individually, passing the full vehicle list so each
    // vehicle can check for others in its forward path (collision avoidance).
    for (Vehicle& vehicle : vehicles) {
        vehicle.update(deltaTime, trafficLights, vehicles, allRoutes);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// reset — Return all vehicles to the start of their routes
// ─────────────────────────────────────────────────────────────────────────────

void VehicleController::reset() {
    // Reset each vehicle independently back to its route starting position.
    for (Vehicle& vehicle : vehicles) {
        vehicle.reset(); // Each vehicle resets its position to route waypoint 0.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Return all vehicles (read-only) for the Renderer to draw. */
const std::vector<Vehicle>& VehicleController::getVehicles() const {
    return vehicles;
}

/** @brief Returns true if at least one vehicle has been spawned. */
bool VehicleController::hasVehicles() const {
    return !vehicles.empty();
}