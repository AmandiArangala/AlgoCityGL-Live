/**
 * @file VehicleController.h
 * @brief Declares VehicleController — spawns and ticks all vehicles in the simulation.
 *
 * Responsibilities
 * ─────────────────
 * 1. **Initialization** — on area load, reads VehicleRoute data and creates
 *    Vehicle instances.  Staggers their starting positions so that roads appear
 *    pre-populated rather than having all vehicles start at waypoint 0.
 *
 * 2. **Per-frame update** — advances every vehicle's position, handles red-light
 *    stopping, and performs basic collision avoidance.
 *
 * 3. **Density control** — on very large maps (> 150 routes), spawns only 25%
 *    of routes to keep frame rate acceptable.
 *
 * Vehicle lifetime
 * ─────────────────
 * Vehicles loop along their route indefinitely.  When a vehicle reaches the
 * final waypoint it wraps back to waypoint 0, making the traffic simulation
 * continuous without needing to spawn or despawn vehicles.
 */

#pragma once

#include <vector>
#include "Vehicle.h"
#include "CityArea.h"
#include "SignalController.h" // For RuntimeTrafficLight.

class VehicleController {
public:
    /**
     * @brief Spawn vehicles for all routes in the given city area.
     *
     * For each VehicleRoute with ≥ 2 waypoints, one or more Vehicle objects are
     * created and staggered along the route (spaced ~8 simulation-seconds apart).
     *
     * On very dense maps (> 150 routes) only 25% of routes get vehicles to
     * maintain rendering performance.
     *
     * @param area  The loaded city area containing route data.
     */
    void initializeFromArea(const CityArea& area);

    /**
     * @brief Advance all vehicles by deltaTime seconds.
     *
     * Does nothing if `isPlaying` is false (simulation paused).
     * Clamps deltaTime to a safe range [0, 1s] to prevent large jumps
     * if the frame rate drops unexpectedly.
     *
     * @param deltaTime    Seconds to advance (should be 1/60 × speed multiplier).
     * @param isPlaying    True if the simulation is running; false if paused.
     * @param trafficLights  Current state of all traffic lights (for stop logic).
     */
    void update(
        float deltaTime,
        bool isPlaying,
        const std::vector<RuntimeTrafficLight>& trafficLights
    );

    /** @brief Reset all vehicles to the start of their routes. */
    void reset();

    /**
     * @brief Return all vehicles (read-only) for rendering.
     *
     * The Renderer iterates this list to draw each vehicle's transformed polygon.
     */
    const std::vector<Vehicle>& getVehicles() const;

    /** @brief Returns true if at least one vehicle has been spawned. */
    bool hasVehicles() const;

private:
    std::vector<Vehicle>      vehicles;   ///< All active vehicles in the simulation.
    std::vector<VehicleRoute> allRoutes;  ///< All routes (used for dynamic lane-switching).
};