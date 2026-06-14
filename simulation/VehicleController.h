#pragma once

#include <vector>
#include "Vehicle.h"
#include "CityArea.h"
#include "SignalController.h"

class VehicleController {
public:
    void initializeFromArea(const CityArea& area);
    void update(float deltaTime, bool isPlaying, const std::vector<RuntimeTrafficLight>& trafficLights);
    void reset();

    const std::vector<Vehicle>& getVehicles() const;
    bool hasVehicles() const;

private:
    std::vector<Vehicle> vehicles;
    std::vector<VehicleRoute> allRoutes;
};