/**
 * @file VehicleController.h
 * @brief Declares VehicleController — spawns and ticks all vehicles in the simulation.
 *
 * Responsibilities:
 *  1. initializeFromArea(): reads VehicleRoute data, creates Vehicle objects,
 *     and pre-advances them so they start at staggered positions on the road.
 *  2. update(): calls Vehicle::update() for every vehicle each frame,
 *     passing the simulation delta-time, traffic lights, and sibling vehicles.
 *  3. reset(): resets each vehicle to its route start position.
 */
#pragma once

#include <vector>
#include "Vehicle.h"
#include "CityArea.h"
#include "SignalController.h"

class VehicleController {
public:
    /**
     * @brief Reads routes from the loaded area, creates vehicle agents, and spaces them out.
     * @param area  The currently loaded CityArea.
     */
    void initializeFromArea(const CityArea& area);

    /**
     * @brief Advances all vehicles by deltaTime if the simulation is playing.
     * @param deltaTime    Time step in seconds (clamped to [0, 1]).
     * @param isPlaying    Vehicles only move when this is true.
     * @param trafficLights Current state of all traffic lights (for stop logic).
     */
    void update(float deltaTime, bool isPlaying, const std::vector<RuntimeTrafficLight>& trafficLights);

    /** @brief Resets every vehicle to its route starting position. */
    void reset();

    /** @brief Returns a const reference to all vehicle objects for rendering. */
    const std::vector<Vehicle>& getVehicles() const;

    /** @brief Returns true if at least one vehicle exists. */
    bool hasVehicles() const;

private:
    std::vector<Vehicle>      vehicles;   ///< All active vehicle agents.
    std::vector<VehicleRoute> allRoutes;  ///< All routes in the area (used for junction routing).
};