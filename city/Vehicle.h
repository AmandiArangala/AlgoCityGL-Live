/**
 * @file Vehicle.h
 * @brief Declares the Vehicle class — a single moving entity in the traffic simulation.
 *
 * Vehicle types (randomly assigned at construction):
 *   CAR   — smallest, fastest (80–99 world units/s)
 *   BUS   — longest, slowest (50–64 world units/s)
 *   TRUCK — medium size, medium speed (55–69 world units/s)
 *   BIKE  — narrowest, moderate speed (70–89 world units/s)
 *
 * Movement model:
 *   - Follows a list of Vec2 waypoints (the assigned VehicleRoute).
 *   - Steers toward the next waypoint by interpolating the heading angle.
 *   - At junctions (where routes overlap), picks a random connected route.
 *   - Stops for red traffic lights and maintains a safe following distance.
 *
 * Transform pipeline (per frame):
 *   localVertices → scale → rotation → laneOffset → translation = transformedVertices
 */
#pragma once

#include <vector>
#include "CityArea.h"
#include "Matrix3x3.h"
#include "SignalController.h"

class Vehicle {
public:
    /** @brief Possible vehicle types — each has different geometry and speed. */
    enum Type { CAR, BUS, TRUCK, BIKE };

    /**
     * @brief Constructs a vehicle, randomly selects its type, and defines local vertices.
     *
     * The type determines the size of the local bounding rectangle and the base speed.
     */
    Vehicle();

    /**
     * @brief Assigns a route and places the vehicle at the first waypoint.
     * @param routePoints  Ordered list of world-space waypoints.
     */
    void setRoute(const std::vector<Vec2>& routePoints);

    /**
     * @brief Advances the vehicle by deltaTime seconds.
     *
     * In order:
     *  1. Check for red-light stop condition.
     *  2. Check for a vehicle directly ahead (collision avoidance).
     *  3. Steer the heading angle toward the current target waypoint.
     *  4. Move forward at current speed.
     *  5. At junctions, randomly choose a connected route.
     *  6. Rebuild the transform matrix and transformed vertices.
     *
     * @param deltaTime     Time step in seconds.
     * @param trafficLights Current state of all lights for stop detection.
     * @param otherVehicles All other vehicles for proximity checks.
     * @param allRoutes     All routes in the area for junction routing.
     */
    void update(
        float deltaTime,
        const std::vector<RuntimeTrafficLight>& trafficLights,
        const std::vector<Vehicle>& otherVehicles,
        const std::vector<VehicleRoute>& allRoutes
    );

    /** @brief Resets position, angle, and state back to the first waypoint. */
    void reset();

    /** @brief Returns world-space vertices after the current transform is applied. */
    const std::vector<Vec2>& getTransformedVertices() const;

    Vec2        getPosition()          const; ///< World-space position of the vehicle centre.
    float       getAngle()             const; ///< Current heading in degrees.
    float       getSpeed()             const; ///< Current speed in world units per second.
    bool        getIsStopped()         const; ///< True when halted (red light or vehicle ahead).
    int         getCurrentTargetIndex()const; ///< Index of the current target waypoint.
    int         getRouteSize()         const; ///< Total number of waypoints in the route.
    float       getOpacity()           const; ///< Visual opacity [0,1] (currently always 1).
    Matrix3x3   getTransformMatrix()   const; ///< The full 3x3 world transform.
    Type        getType()              const; ///< Vehicle type (CAR / BUS / TRUCK / BIKE).

private:
    std::vector<Vec2> route;               ///< Assigned waypoint list.
    std::vector<Vec2> localVertices;       ///< Shape in local (un-transformed) space.
    std::vector<Vec2> transformedVertices; ///< Shape after the current world transform.

    Type  type;          ///< Randomly chosen vehicle type.

    Vec2  position;      ///< Current world-space position.
    float angleDegrees;  ///< Current heading angle in degrees.
    float speed;         ///< Current speed in world units per second.
    float opacity = 1.0f;///< Visual opacity (1.0 = fully visible).

    int  currentTargetIndex; ///< Index of the next waypoint to head toward.
    bool routeReady;         ///< True when a valid route (\u22652 points) is assigned.
    bool stoppedAtRedLight;  ///< True when halted for a red light or blocked by another vehicle.

    Matrix3x3 transformMatrix; ///< Combined 3x3 transform applied to localVertices each frame.

    /** @brief Recomputes transformMatrix and transformedVertices from current state. */
    void updateTransform();

    /**
     * @brief Checks whether any nearby red light governs this vehicle's direction.
     * @return true if the vehicle should stop at a red signal.
     */
    bool shouldStopForRedLight(const std::vector<RuntimeTrafficLight>& trafficLights) const;

    /** @brief Euclidean distance between two world-space points. */
    float distance(const Vec2& a, const Vec2& b) const;

    /** @brief Returns a normalised (unit-length) copy of v; returns (0,0) if v is zero. */
    Vec2 normalize(const Vec2& v) const;
};