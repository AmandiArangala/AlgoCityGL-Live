/**
 * @file Vehicle.h
 * @brief Declares the Vehicle class — a single moving entity in the traffic simulation.
 *
 * Vehicle types
 * ──────────────
 * Each vehicle is randomly assigned one of four types at construction:
 *
 *  Type   | Body size (half-extents) | Speed range
 *  -------|--------------------------|------------
 *  CAR    | 18 × 10                  | 80–99 units/s
 *  BUS    | 28 × 11                  | 50–64 units/s
 *  TRUCK  | 24 × 12                  | 55–69 units/s
 *  BIKE   |  8 ×  4                  | 70–89 units/s
 *
 * Body representation
 * ────────────────────
 * Each vehicle has a rectangular body defined by four `localVertices` in
 * vehicle-local space (centred at the origin, aligned with the forward axis).
 * Every frame, updateTransform() computes a composed Matrix3x3:
 *
 *   transformMatrix = Translation * Rotation * LaneOffset * Scale
 *
 * and applies it to `localVertices` to produce `transformedVertices` in
 * world-space, which the Renderer draws as a filled polygon.
 *
 * Movement model
 * ──────────────
 * Vehicles follow a list of waypoints (their `route`).  Each frame, update():
 *  1. Checks for a red light ahead → stop.
 *  2. Checks for another vehicle too close ahead → stop (collision avoidance).
 *  3. Steers toward the next waypoint (smooth angular interpolation).
 *  4. Moves forward at `speed` units/s.
 *  5. When nearing a waypoint, evaluates connecting routes and possibly switches
 *     lanes (dynamic routing at junctions).
 *
 * Coordinate units
 * ─────────────────
 * All positions are in scaled world-space pixels (JSON coordinates × SPREAD_FACTOR).
 * Speed is in those pixels per second; typical road widths are ~30–50 units.
 */

#pragma once

#include <vector>
#include "CityArea.h"         // For Vec2, VehicleRoute.
#include "Matrix3x3.h"        // For transform composition.
#include "SignalController.h" // For RuntimeTrafficLight.

class Vehicle {
public:
    /** @brief The four available vehicle types with different sizes and speeds. */
    enum Type { CAR, BUS, TRUCK, BIKE };

    /**
     * @brief Construct a vehicle with a randomly assigned type and body geometry.
     *
     * The type is chosen using std::rand() % 4.  Body vertices and speed are
     * set according to the type table in the class description.
     */
    Vehicle();

    // ── Route Control ─────────────────────────────────────────────────────────

    /**
     * @brief Assign a route and place the vehicle at the first waypoint.
     *
     * Also sets the initial heading angle to face the second waypoint.
     * Calls updateTransform() to apply the new position immediately.
     *
     * @param routePoints  Ordered list of world-space waypoints.
     */
    void setRoute(const std::vector<Vec2>& routePoints);

    /** @brief Reset position and heading to route waypoint 0 (for replay). */
    void reset();

    // ── Per-frame Update ──────────────────────────────────────────────────────

    /**
     * @brief Advance the vehicle by deltaTime seconds.
     *
     * Performs (in order):
     *  1. Red-light stop check.
     *  2. Collision avoidance with vehicles ahead.
     *  3. Steering toward next waypoint (angular interpolation).
     *  4. Forward movement.
     *  5. Waypoint advance / dynamic route switching.
     *  6. Transform update.
     *
     * @param deltaTime      Time step in seconds.
     * @param trafficLights  All current traffic lights (for stop logic).
     * @param otherVehicles  All other vehicles (for collision avoidance).
     * @param allRoutes      All routes (for dynamic junction switching).
     */
    void update(
        float deltaTime,
        const std::vector<RuntimeTrafficLight>& trafficLights,
        const std::vector<Vehicle>& otherVehicles,
        const std::vector<VehicleRoute>& allRoutes
    );

    // ── Getters ───────────────────────────────────────────────────────────────

    /** @brief Return the world-space transformed body polygon (4 vertices). */
    const std::vector<Vec2>& getTransformedVertices() const;

    Vec2        getPosition()           const; ///< World-space position (centre of body).
    float       getAngle()              const; ///< Current heading in degrees.
    float       getSpeed()              const; ///< Speed in world-units per second.
    bool        getIsStopped()          const; ///< True if stopped (red light or blocked).
    int         getCurrentTargetIndex() const; ///< Index of the next waypoint in `route`.
    int         getRouteSize()          const; ///< Total number of waypoints in `route`.
    float       getOpacity()            const; ///< Visual opacity [0..1] (unused fade-in).
    Matrix3x3   getTransformMatrix()    const; ///< The full composed transform matrix.
    Type        getType()               const; ///< Vehicle type (CAR / BUS / TRUCK / BIKE).

private:
    // ── Route Data ────────────────────────────────────────────────────────────
    std::vector<Vec2> route;              ///< Active route waypoints.
    std::vector<Vec2> localVertices;      ///< Body corners in local (vehicle) space.
    std::vector<Vec2> transformedVertices;///< Body corners transformed to world space.

    // ── State ─────────────────────────────────────────────────────────────────
    Type  type;                           ///< Vehicle type (set once in constructor).
    Vec2  position;                       ///< World-space position (body centre).
    float angleDegrees;                   ///< Heading angle in degrees (0 = right, 90 = down).
    float speed;                          ///< Movement speed in world-units per second.
    float opacity = 1.0f;                 ///< Opacity for potential fade-in effects.

    int   currentTargetIndex;             ///< Index of the next waypoint to steer towards.
    bool  routeReady;                     ///< True once setRoute() has been called successfully.
    bool  stoppedAtRedLight;              ///< True when halted (red light OR vehicle ahead).

    Matrix3x3 transformMatrix;            ///< Composed world-space transform (rebuilt each frame).

    // ── Private Helpers ───────────────────────────────────────────────────────

    /**
     * @brief Recompute transformedVertices from localVertices using transformMatrix.
     *
     * Called at the end of every update() call and after setRoute()/reset().
     */
    void updateTransform();

    /**
     * @brief Check whether this vehicle should stop for any nearby red light.
     *
     * A vehicle stops if:
     *  - A red light is within 60 units.
     *  - The light governs this vehicle's direction (dot-product > 0.8).
     *  - The light is ahead of the vehicle (not behind).
     */
    bool shouldStopForRedLight(const std::vector<RuntimeTrafficLight>& trafficLights) const;

    /** @brief Compute Euclidean distance between two Vec2 points. */
    float distance(const Vec2& a, const Vec2& b) const;

    /** @brief Return the unit vector in the direction of v (zero vector if v is zero). */
    Vec2 normalize(const Vec2& v) const;
};