/**
 * @file Matrix3x3.cpp
 * @brief Implements 3×3 homogeneous transformation matrices for 2D transforms.
 *
 * Homogeneous coordinates recap
 * ──────────────────────────────
 * A 2D point (x, y) is extended to (x, y, 1) in homogeneous form.
 * This allows translation, rotation, and scaling to all be represented as
 * 3×3 matrix multiplications, enabling them to be combined into one matrix:
 *
 *   Combined = T * R * S   (scale first, rotate, then translate)
 *
 * Applying the combined matrix once is faster than applying three separate
 * transforms sequentially, especially when the same matrix is reused for
 * all vertices of a vehicle each frame.
 */

#include "Matrix3x3.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — Initialize to Identity
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Construct a 3×3 identity matrix.
 *
 * The identity matrix represents "no transform":
 *   | 1  0  0 |
 *   | 0  1  0 |
 *   | 0  0  1 |
 */
Matrix3x3::Matrix3x3() {
    // Initialise to the identity matrix:
    //   [ 1 0 0 ]
    //   [ 0 1 0 ]
    //   [ 0 0 1 ]
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory — Identity
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Create and return a fresh identity matrix. */
Matrix3x3 Matrix3x3::identity() {
    Matrix3x3 result;
    // Same as the default constructor, but returned as a named value.
    result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f;
    result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f;
    result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f;

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory — Translation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a translation matrix that moves points by (tx, ty).
 *
 * Matrix form:
 *   | 1  0  tx |
 *   | 0  1  ty |
 *   | 0  0   1 |
 *
 * When multiplied with a homogeneous point (x, y, 1):
 *   x' = x + tx,  y' = y + ty
 */
Matrix3x3 Matrix3x3::translation(float tx, float ty) {
    Matrix3x3 result = Matrix3x3::identity();

    result.m[0][2] = tx; // Translation in x goes in the last column, row 0.
    result.m[1][2] = ty; // Translation in y goes in the last column, row 1.

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory — Rotation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a 2D rotation matrix for `angleDegrees` degrees.
 *
 * Matrix form (counter-clockwise, standard math convention):
 *   | cos θ  -sin θ  0 |
 *   | sin θ   cos θ  0 |
 *   |   0       0    1 |
 *
 * Note: because screen-space has Y increasing downward, a positive angle
 * produces a clockwise visual rotation in the simulator.
 */
Matrix3x3 Matrix3x3::rotation(float angleDegrees) {
    Matrix3x3 result = Matrix3x3::identity();

    float radians = angleDegrees * 3.14159265f / 180.0f; // Convert degrees → radians.
    float c = std::cos(radians);
    float s = std::sin(radians);

    result.m[0][0] =  c; // cos θ
    result.m[0][1] = -s; // -sin θ
    result.m[1][0] =  s; // sin θ
    result.m[1][1] =  c; // cos θ

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory — Scaling
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a non-uniform scaling matrix.
 *
 * Matrix form:
 *   | sx  0   0 |
 *   |  0  sy  0 |
 *   |  0   0  1 |
 *
 * sx and sy can differ to stretch or squash independently on each axis.
 * Use sx = sy for uniform scaling.
 */
Matrix3x3 Matrix3x3::scaling(float sx, float sy) {
    Matrix3x3 result = Matrix3x3::identity();

    result.m[0][0] = sx; // Scale x.
    result.m[1][1] = sy; // Scale y.

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// operator* — Matrix Multiplication (composition of transforms)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Multiply this matrix by `other` and return the product.
 *
 * Standard matrix multiplication: C[i][j] = Σ_k A[i][k] * B[k][j]
 *
 * The order matters:  (A * B) applied to a point transforms it by B first,
 * then by A.  In the Vehicle, the order is:
 *
 *   transformMatrix = Translation * Rotation * LaneOffset * Scale
 *
 * So a vertex is: scaled → lane-offset → rotated → translated.
 */
Matrix3x3 Matrix3x3::operator*(const Matrix3x3& other) const {
    Matrix3x3 result;

    // Standard 3x3 matrix multiplication: result[r][c] = sum over k of m[r][k] * other[k][c].
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            result.m[row][col] = 0.0f;

            // Dot product of row `row` from `this` with column `col` from `other`.
            for (int k = 0; k < 3; k++) {
                result.m[row][col] += m[row][k] * other.m[k][col];
            }
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// transformPoint — Apply the matrix to a 2D point
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Apply this matrix to a homogeneous 2D point (x, y, 1).
 *
 * Only rows 0 and 1 are evaluated; row 2 is always [0, 0, 1] for affine
 * transforms, so the w component stays 1 and can be ignored.
 *
 * Result:
 *   x' = m[0][0]*x + m[0][1]*y + m[0][2]
 *   y' = m[1][0]*x + m[1][1]*y + m[1][2]
 */
Vec2 Matrix3x3::transformPoint(const Vec2& point) const {
    float x = m[0][0] * point.x + m[0][1] * point.y + m[0][2]; // x' from row 0.
    float y = m[1][0] * point.x + m[1][1] * point.y + m[1][2]; // y' from row 1.

    return Vec2(x, y);
}