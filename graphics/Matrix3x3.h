/**
 * @file Matrix3x3.h
 * @brief Declares a 3×3 homogeneous transformation matrix for 2D graphics.
 *
 * Why homogeneous coordinates?
 * ─────────────────────────────
 * In 2D, translations cannot be expressed as a simple 2×2 matrix multiplication.
 * Homogeneous coordinates add a third component w=1 to every point, lifting
 * the math to 3D.  This lets translation, rotation, and scaling all be expressed
 * as 3×3 matrices that can be combined (multiplied together) into a single
 * compound transform.
 *
 * The general homogeneous 3×3 matrix layout:
 *
 *   | m[0][0]  m[0][1]  m[0][2] |   | sx·cos θ  -sy·sin θ  tx |
 *   | m[1][0]  m[1][1]  m[1][2] | = | sx·sin θ   sy·cos θ  ty |
 *   | m[2][0]  m[2][1]  m[2][2] |   |     0          0       1 |
 *
 * Usage in AlgoCityGL
 * ────────────────────
 * Each Vehicle builds its own transformMatrix by composing:
 *   Translation * Rotation * LaneOffset * Scale
 * then calls transformPoint() on each of its four body vertices to obtain
 * their world-space positions for rendering.
 */

#pragma once

#include "CityArea.h" // For Vec2.

class Matrix3x3 {
public:
    float m[3][3]; ///< Row-major storage: m[row][col].

    /** @brief Construct a 3×3 identity matrix. */
    Matrix3x3();

    // ── Factory methods ───────────────────────────────────────────────────────

    /** @brief Return the 3×3 identity matrix (no transform). */
    static Matrix3x3 identity();

    /**
     * @brief Return a translation matrix that shifts by (tx, ty).
     *
     *   | 1  0  tx |
     *   | 0  1  ty |
     *   | 0  0   1 |
     */
    static Matrix3x3 translation(float tx, float ty);

    /**
     * @brief Return a counter-clockwise rotation matrix for the given angle.
     *
     * @param angleDegrees  Rotation angle in degrees (positive = counter-clockwise).
     *
     *   | cos θ  -sin θ  0 |
     *   | sin θ   cos θ  0 |
     *   |   0       0    1 |
     */
    static Matrix3x3 rotation(float angleDegrees);

    /**
     * @brief Return a non-uniform scaling matrix.
     *
     * @param sx  Horizontal scale factor.
     * @param sy  Vertical scale factor.
     *
     *   | sx  0   0 |
     *   |  0  sy  0 |
     *   |  0   0  1 |
     */
    static Matrix3x3 scaling(float sx, float sy);

    // ── Operations ────────────────────────────────────────────────────────────

    /**
     * @brief Multiply two 3×3 matrices (standard matrix multiplication).
     *
     * Used to compose transforms: (A * B) applies B first, then A.
     * E.g., Translation * Rotation applies rotation first, then translation.
     *
     * @return A new Matrix3x3 that is the product of `this` × `other`.
     */
    Matrix3x3 operator*(const Matrix3x3& other) const;

    /**
     * @brief Transform a 2D point by this matrix.
     *
     * The point (x, y) is treated as a homogeneous column vector (x, y, 1)ᵀ.
     * The result is:
     *   x' = m[0][0]*x + m[0][1]*y + m[0][2]
     *   y' = m[1][0]*x + m[1][1]*y + m[1][2]
     * (The third row is always [0, 0, 1] so the w component is ignored.)
     *
     * @param point  Input 2D point.
     * @return       Transformed 2D point.
     */
    Vec2 transformPoint(const Vec2& point) const;
};