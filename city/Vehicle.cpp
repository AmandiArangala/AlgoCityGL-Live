/**
 * @file Vehicle.cpp
 * @brief Implements Vehicle — movement, collision avoidance, and transform updates.
 *
 * Key algorithms used
 * ────────────────────
 * 1. **Route following** — linear interpolation between waypoints with smooth
 *    angular steering (capped turn speed prevents instant direction changes).
 *
 * 2. **Red-light detection** — dot-product test between the vehicle's forward
 *    vector and the vector to each traffic light to check if the light is ahead.
 *    A direction field on each light allows lane-specific signals.
 *
 * 3. **Collision avoidance** — dot-product test between the forward vector and
 *    the vector to each other vehicle.  If a same-direction vehicle is within
 *    70 units and directly ahead (dot > 0.8), this vehicle halts.
 *
 * 4. **Dynamic lane switching** — near each waypoint, the vehicle searches
 *    `allRoutes` for a connecting segment that doesn't require a U-turn
 *    (dot-product > -0.5) and randomly picks one.  This gives vehicles
 *    emergent turn behaviour at junctions without explicit pathfinding.
 *
 * 5. **Matrix3x3 transform** — each frame, a composed 3×3 homogeneous matrix
 *    (Translation × Rotation × LaneOffset × Scale) is applied to the four
 *    rectangular body vertices to obtain their world-space positions.
 */

#include "Vehicle.h"
#include <cmath>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — Assign type, build body geometry, set initial speed
// ─────────────────────────────────────────────────────────────────────────────

Vehicle::Vehicle()
    : position(0.0f, 0.0f),
      angleDegrees(0.0f),
      speed(80.0f),
      currentTargetIndex(1),
      routeReady(false),
      stoppedAtRedLight(false) {

    // Randomly assign one of four vehicle types.
    type = static_cast<Type>(std::rand() % 4);

    // Define the four corners of the vehicle body in local (vehicle-centred) space.
    // Positive x = forward, positive y = right (before rotation).
    // The values are half-extents: the body spans [-w, +w] × [-h, +h].
    if (type == CAR) {
        localVertices.push_back(Vec2(-18.0f, -10.0f)); // Rear-left
        localVertices.push_back(Vec2(18.0f, -10.0f));  // Front-left
        localVertices.push_back(Vec2(18.0f, 10.0f));   // Front-right
        localVertices.push_back(Vec2(-18.0f, 10.0f));  // Rear-right
        speed = 80.0f + (std::rand() % 20); // 80–99 units/s
    } else if (type == BUS) {
        localVertices.push_back(Vec2(-28.0f, -11.0f));
        localVertices.push_back(Vec2(28.0f, -11.0f));
        localVertices.push_back(Vec2(28.0f, 11.0f));
        localVertices.push_back(Vec2(-28.0f, 11.0f));
        speed = 50.0f + (std::rand() % 15); // 50–64 units/s (larger, slower)
    } else if (type == TRUCK) {
        localVertices.push_back(Vec2(-24.0f, -12.0f));
        localVertices.push_back(Vec2(24.0f, -12.0f));
        localVertices.push_back(Vec2(24.0f, 12.0f));
        localVertices.push_back(Vec2(-24.0f, 12.0f));
        speed = 55.0f + (std::rand() % 15); // 55–69 units/s
    } else if (type == BIKE) {
        localVertices.push_back(Vec2(-8.0f, -4.0f));
        localVertices.push_back(Vec2(8.0f, -4.0f));
        localVertices.push_back(Vec2(8.0f, 4.0f));
        localVertices.push_back(Vec2(-8.0f, 4.0f));
        speed = 70.0f + (std::rand() % 20); // 70–89 units/s (small, nimble)
    }

    updateTransform(); // Compute initial world-space vertices at the origin.
}

// ─────────────────────────────────────────────────────────────────────────────
// setRoute — Assign a route and place the vehicle at waypoint 0
// ─────────────────────────────────────────────────────────────────────────────

void Vehicle::setRoute(const std::vector<Vec2>& routePoints) {
    route = routePoints;

    if (route.size() >= 2) {
        position          = route[0]; // Start at the first waypoint.
        currentTargetIndex = 1;        // Next target is waypoint 1.
        routeReady        = true;
        stoppedAtRedLight = false;

        // Compute initial heading: angle from waypoint 0 toward waypoint 1.
        Vec2 direction(route[1].x - route[0].x, route[1].y - route[0].y);

        if (distance(route[0], route[1]) > 0.0f) {
            // atan2 returns the angle in radians; convert to degrees for our matrix.
            angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
        }
    } else {
        routeReady = false; // Cannot follow a route with fewer than 2 points.
    }

    updateTransform();
}

// ─────────────────────────────────────────────────────────────────────────────
// reset — Return to waypoint 0 for replay / restart
// ─────────────────────────────────────────────────────────────────────────────

void Vehicle::reset() {
    if (route.size() >= 2) {
        position           = route[0];
        currentTargetIndex = 1;
        routeReady         = true;
        stoppedAtRedLight  = false;

        Vec2 direction(route[1].x - route[0].x, route[1].y - route[0].y);
        angleDegrees = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
    }

    updateTransform();
}

// ─────────────────────────────────────────────────────────────────────────────
// update — Main per-frame vehicle logic
// ─────────────────────────────────────────────────────────────────────────────

void Vehicle::update(
    float deltaTime,
    const std::vector<RuntimeTrafficLight>& trafficLights,
    const std::vector<Vehicle>& otherVehicles,
    const std::vector<VehicleRoute>& allRoutes
) {
    // Guard: don't move if no route is set or the vehicle is invisible.
    if (!routeReady || opacity <= 0.0f || route.size() < 2) {
        return;
    }

    // ── Step 1: Red-light check ───────────────────────────────────────────────
    if (shouldStopForRedLight(trafficLights)) {
        stoppedAtRedLight = true;
        updateTransform(); // Refresh transform even when stopped (angle may have changed).
        return;
    }

    // ── Step 2: Collision avoidance ───────────────────────────────────────────
    bool vehicleAhead = false;
    float safeDistance = 70.0f; // Gap to maintain between vehicles (world units).

    // Compute the forward direction towards the current target waypoint.
    Vec2 direction = (currentTargetIndex < route.size())
        ? Vec2(route[currentTargetIndex].x - position.x, route[currentTargetIndex].y - position.y)
        : Vec2(0.0f, 0.0f);
    Vec2 forward = normalize(direction);

    for (const Vehicle& other : otherVehicles) {
        if (&other == this) continue; // Skip self-comparison.

        float d = distance(position, other.getPosition());
        if (d < safeDistance) {
            Vec2 toOther(other.getPosition().x - position.x, other.getPosition().y - position.y);
            Vec2 toOtherDir = normalize(toOther);

            // Check if the other vehicle is in front (dot product > 0.8 → ~37° cone).
            float dotProduct = forward.x * toOtherDir.x + forward.y * toOtherDir.y;
            if (dotProduct > 0.8f) {
                // Ensure they are roughly moving in same direction (not oncoming traffic crossing paths).
                Vec2 otherForward(
                    std::cos(other.getAngle() * 3.14159f / 180.0f),
                    std::sin(other.getAngle() * 3.14159f / 180.0f)
                );
                if (forward.x * otherForward.x + forward.y * otherForward.y > 0.5f) {
                    vehicleAhead = true;
                    break;
                }
            }
        }
    }

    if (vehicleAhead) {
        stoppedAtRedLight = true; // Reuse the "stopped" visual state for being blocked.
        updateTransform();
        return;
    }

    stoppedAtRedLight = false; // Clear stopped flag; vehicle is free to move.

    // ── Step 3 & 4: Move toward waypoints ────────────────────────────────────
    float moveDist = speed * deltaTime; // Total distance to travel this frame.

    // The inner loop consumes moveDist across multiple waypoints in one frame
    // if the vehicle is very close to the next waypoint.
    while (moveDist > 0.0f) {
        // If we've passed the last waypoint, loop back to the start of the route.
        if (currentTargetIndex >= static_cast<int>(route.size())) {
            currentTargetIndex = 1;
            position = route[0];
            opacity  = 1.0f; // Restore opacity (in case fade-in was used).
            break;
        }

        Vec2  target = route[currentTargetIndex];
        float dist   = distance(position, target);

        // ── Step 5: Waypoint advance + dynamic lane switching ──────────────
        // When close enough to a waypoint (~25 units), decide on the next segment.
        if (dist < 25.0f) {
            Vec2 prev = (currentTargetIndex > 0) ? route[currentTargetIndex - 1] : position;

            // Collect candidate next segments: either continue on current route
            // or switch to a connecting route at this junction point.
            struct RouteOption {
                std::vector<Vec2> routePoints;
                int nextIndex;
            };
            std::vector<RouteOption> options;

            // Option: continue on the current route to the next waypoint.
            if (currentTargetIndex + 1 < static_cast<int>(route.size())) {
                options.push_back({route, currentTargetIndex + 1});
            }

            // Search all other routes for a segment that starts near this waypoint.
            for (const VehicleRoute& vr : allRoutes) {
                // Skip the vehicle's own current route to avoid redundant options.
                if (vr.points.size() == route.size() && !vr.points.empty() && !route.empty()
                    && vr.points[0].x == route[0].x && vr.points[0].y == route[0].y) {
                    continue;
                }

                for (size_t i = 0; i + 1 < vr.points.size(); i++) {
                    float d = distance(target, vr.points[i]);
                    if (d < 15.0f) { // Junction detected: vr.points[i] is near our current waypoint.
                        Vec2 optionDir(vr.points[i+1].x - vr.points[i].x, vr.points[i+1].y - vr.points[i].y);
                        Vec2 currentDir(target.x - prev.x, target.y - prev.y);

                        float curLen = std::sqrt(currentDir.x*currentDir.x + currentDir.y*currentDir.y);
                        float optLen = std::sqrt(optionDir.x*optionDir.x + optionDir.y*optionDir.y);

                        if (curLen > 0.001f && optLen > 0.001f) {
                            float dotProd  = optionDir.x * currentDir.x + optionDir.y * currentDir.y;
                            float cosTheta = dotProd / (curLen * optLen);
                            // Accept this route if it doesn't require a U-turn (cosTheta > -0.5).
                            if (cosTheta > -0.5f) { // Allow turns, prevent U-turns
                                options.push_back({vr.points, static_cast<int>(i + 1)});
                            }
                        }
                    }
                }
            }

            // Randomly pick one of the available route options (uniform distribution).
            if (!options.empty()) {
                int choice = std::rand() % options.size();
                route              = options[choice].routePoints;
                currentTargetIndex = options[choice].nextIndex;
            } else {
                currentTargetIndex++; // No junction found; advance to next waypoint.
            }
            continue; // Re-evaluate with the new target.
        }

        // ── Steering: smooth angular interpolation toward the target ──────────
        Vec2  dir2target(target.x - position.x, target.y - position.y);
        float targetAngle = std::atan2(dir2target.y, dir2target.x) * 180.0f / 3.14159265f;

        // Compute the angular difference and normalise to [-180°, +180°].
        float diff = targetAngle - angleDegrees;
        while (diff <= -180.0f) diff += 360.0f;
        while (diff >   180.0f) diff -= 360.0f;

        // Limit the turn per frame based on how far the vehicle moves this step.
        float turnSpeed = 120.0f;                         // Max turn rate: degrees per second.
        float maxTurn   = turnSpeed * (moveDist / speed); // Max degrees to turn this frame.

        if (std::abs(diff) <= maxTurn) {
            angleDegrees = targetAngle; // Snap to target if within the allowed turn.
        } else {
            angleDegrees += (diff > 0 ? maxTurn : -maxTurn); // Turn as fast as allowed.
        }

        // ── Movement: advance along the current heading ───────────────────────
        float rad = angleDegrees * 3.14159265f / 180.0f;
        position.x += std::cos(rad) * moveDist; // Move in the x direction.
        position.y += std::sin(rad) * moveDist; // Move in the y direction.
        moveDist = 0.0f; // All movement consumed for this frame.
    }

    updateTransform(); // Recompute world-space vertices from the new position and angle.
}

// ─────────────────────────────────────────────────────────────────────────────
// shouldStopForRedLight — Test if any red light governs this vehicle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Determine whether this vehicle must stop for a red traffic light.
 *
 * Conditions for stopping (all must be true):
 *  1. The light is within 60 world units of the vehicle.
 *  2. The light is in the Red state.
 *  3. The light governs this vehicle's direction:
 *       - If the light has no direction (zero vector): governs all vehicles.
 *       - Otherwise: vehicle's forward dot light.direction > 0.8.
 *  4. The light is in front of the vehicle (forward · toLight > 0.5).
 */
bool Vehicle::shouldStopForRedLight(const std::vector<RuntimeTrafficLight>& trafficLights) const {
    if (route.empty() || currentTargetIndex >= route.size()) {
        return false;
    }

    // Compute this vehicle's forward unit vector.
    Vec2 direction(route[currentTargetIndex].x - position.x, route[currentTargetIndex].y - position.y);
    Vec2 forward = normalize(direction);

    for (const RuntimeTrafficLight& light : trafficLights) {
        float d = distance(position, light.baseLight.position);

        bool nearLight = d < 60.0f; // Detection radius: 60 world units.

        if (nearLight && light.state == SignalState::Red) {
            bool governsDirection = false;

            if (light.baseLight.direction.x == 0.0f && light.baseLight.direction.y == 0.0f) {
                // Backward compatibility: no direction means governs all vehicles.
                governsDirection = true;
            } else {
                // Directional light: only stop if moving roughly the same way as the light governs.
                float lightDotProduct = forward.x * light.baseLight.direction.x
                                      + forward.y * light.baseLight.direction.y;
                if (lightDotProduct > 0.8f) { // ≈ within 37° of the governed direction.
                    governsDirection = true;
                }
            }

            if (governsDirection) {
                // Final check: the light must be ahead of the vehicle, not behind.
                Vec2 toLight(light.baseLight.position.x - position.x,
                             light.baseLight.position.y - position.y);
                Vec2  toLightDir  = normalize(toLight);
                float dotProduct  = forward.x * toLightDir.x + forward.y * toLightDir.y;

                if (dotProduct > 0.5f) { // Light is in the forward half-space.
                    return true;
                }
            }
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// updateTransform — Compose the 3×3 matrix and apply to body vertices
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Rebuild transformedVertices from the current position, angle, and body shape.
 *
 * Transform order (right-to-left application):
 *   1. Scale (identity here — kept for future scaling features).
 *   2. Lane offset: shift 16 units left (perpendicular to forward) so vehicles
 *      drive on the correct side of the road rather than down the centre line.
 *   3. Rotation: rotate by angleDegrees around the body centre.
 *   4. Translation: move to the world-space position.
 *
 * The lane offset of -16 units in local Y places the vehicle in the
 * left lane when Y increases to the right of the heading direction.
 */
void Vehicle::updateTransform() {
    Matrix3x3 scale      = Matrix3x3::scaling(1.0f, 1.0f);       // No scaling.
    Matrix3x3 rotation   = Matrix3x3::rotation(angleDegrees);     // Rotate to heading.
    // Offset vehicles to the left lane (approx 16 units)
    Matrix3x3 laneOffset = Matrix3x3::translation(0.0f, -16.0f);  // Left-lane offset.
    Matrix3x3 translation = Matrix3x3::translation(position.x, position.y); // World position.

    // Compose: Translation × Rotation × LaneOffset × Scale
    // Applied right-to-left: scale → lane offset → rotate → translate.
    transformMatrix = translation * rotation * laneOffset * scale;

    // Apply the combined transform to each local body vertex.
    transformedVertices.clear();
    for (const Vec2& vertex : localVertices) {
        transformedVertices.push_back(transformMatrix.transformPoint(vertex));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility helpers
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Euclidean distance between two 2D points. */
float Vehicle::distance(const Vec2& a, const Vec2& b) const {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Return the unit vector of v.
 *
 * Returns (0, 0) for zero-length vectors to avoid division by zero.
 */
Vec2 Vehicle::normalize(const Vec2& v) const {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len > 0.0f) {
        return Vec2(v.x / len, v.y / len);
    }
    return Vec2(0.0f, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

Vehicle::Type Vehicle::getType()  const { return type; }

const std::vector<Vec2>& Vehicle::getTransformedVertices() const {
    return transformedVertices;
}

Vec2  Vehicle::getPosition()           const { return position; }
float Vehicle::getAngle()              const { return angleDegrees; }
float Vehicle::getSpeed()              const { return speed; }
bool  Vehicle::getIsStopped()          const { return stoppedAtRedLight; }
int   Vehicle::getCurrentTargetIndex() const { return currentTargetIndex; }
int   Vehicle::getRouteSize()          const { return static_cast<int>(route.size()); }
float Vehicle::getOpacity()            const { return opacity; }

Matrix3x3 Vehicle::getTransformMatrix() const { return transformMatrix; }