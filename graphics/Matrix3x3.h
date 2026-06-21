/**
 * @file Matrix3x3.h
 * @brief Declares a 3\u00d73 homogeneous transformation matrix for 2D graphics.
 *
 * Why homogeneous coordinates?
 * In 2D, translation cannot be expressed as a 2x2 matrix multiplication.
 * By adding a third \"W\" dimension and representing 2D points as (x, y, 1),
 * translation, rotation, and scaling can ALL be expressed as 3\u00d73 matrix
 * multiplications and chained together.
 *
 * Standard form of a 2D transform matrix:
 *   [ cos\u03b8  -sin\u03b8   tx ]
 *   [ sin\u03b8   cos\u03b8   ty ]
 *   [  0       0     1  ]
 *
 * Used by Vehicle::updateTransform() to compute each vehicle's
 * world-space vertices from its local shape, speed, and heading angle.
 */
#pragma once

#include "CityArea.h"

class Matrix3x3 {
public:
    float m[3][3]; ///< Row-major 3\u00d73 matrix storage: m[row][col].

    Matrix3x3(); ///< Constructs the identity matrix.

    /** @brief Returns a 3\u00d73 identity matrix (no transformation). */
    static Matrix3x3 identity();

    /**
     * @brief Returns a translation matrix that moves points by (tx, ty).
     * @param tx  Horizontal displacement in world units.
     * @param ty  Vertical displacement in world units.
     */
    static Matrix3x3 translation(float tx, float ty);

    /**
     * @brief Returns a 2D rotation matrix for the given angle.
     * @param angleDegrees  Counter-clockwise rotation in degrees.
     */
    static Matrix3x3 rotation(float angleDegrees);

    /**
     * @brief Returns a uniform or non-uniform scale matrix.
     * @param sx  X scale factor.
     * @param sy  Y scale factor.
     */
    static Matrix3x3 scaling(float sx, float sy);

    /**
     * @brief Matrix multiplication: combines two transforms into one.
     * @return  A new matrix equal to (this * other).
     */
    Matrix3x3 operator*(const Matrix3x3& other) const;

    /**
     * @brief Applies this transform to a 2D point.
     * @param point  Input point in homogeneous 2D (w=1 assumed).
     * @return       Transformed point.
     */
    Vec2 transformPoint(const Vec2& point) const;
};