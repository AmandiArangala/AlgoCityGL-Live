/**
 * @file Matrix3x3.cpp
 * @brief Implements 3x3 homogeneous transformation matrices for 2D transforms.
 *
 * Homogeneous coordinates recap:
 * A 2D point (x, y) is represented as (x, y, 1) so that translation,
 * rotation, and scaling can all be written as 3x3 matrix multiplications
 * and chained with operator*.
 *
 * The combined vehicle transform is built as:
 *   T(position) * R(angleDegrees) * T(laneOffset) * S(scale)
 * applied left-to-right via operator* calls.
 */
#include "Matrix3x3.h"
#include <cmath>

Matrix3x3::Matrix3x3() {
    // Initialise to the identity matrix:
    //   [ 1 0 0 ]
    //   [ 0 1 0 ]
    //   [ 0 0 1 ]
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f;
}

Matrix3x3 Matrix3x3::identity() {
    Matrix3x3 result;
    // Same as the default constructor, but returned as a named value.
    result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f;
    result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f;
    result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f;

    return result;
}

Matrix3x3 Matrix3x3::translation(float tx, float ty) {
    Matrix3x3 result = Matrix3x3::identity();
    // Place the translation components in the right column:
    //   [ 1 0 tx ]
    //   [ 0 1 ty ]
    //   [ 0 0  1 ]
    result.m[0][2] = tx;
    result.m[1][2] = ty;

    return result;
}

Matrix3x3 Matrix3x3::rotation(float angleDegrees) {
    Matrix3x3 result = Matrix3x3::identity();

    // Convert degrees to radians: rad = degrees * pi / 180.
    float radians = angleDegrees * 3.14159265f / 180.0f;
    float c = std::cos(radians); // cos(theta)
    float s = std::sin(radians); // sin(theta)

    // Standard 2D rotation matrix:
    //   [ c  -s  0 ]
    //   [ s   c  0 ]
    //   [ 0   0  1 ]
    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;

    return result;
}

Matrix3x3 Matrix3x3::scaling(float sx, float sy) {
    Matrix3x3 result = Matrix3x3::identity();
    // Place scale factors on the diagonal:
    //   [ sx  0  0 ]
    //   [  0 sy  0 ]
    //   [  0  0  1 ]
    result.m[0][0] = sx;
    result.m[1][1] = sy;

    return result;
}

Matrix3x3 Matrix3x3::operator*(const Matrix3x3& other) const {
    Matrix3x3 result;

    // Standard 3x3 matrix multiplication: result[r][c] = sum over k of m[r][k] * other[k][c].
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            result.m[row][col] = 0.0f;

            for (int k = 0; k < 3; k++) {
                result.m[row][col] += m[row][k] * other.m[k][col];
            }
        }
    }

    return result;
}

Vec2 Matrix3x3::transformPoint(const Vec2& point) const {
    // Apply the 3x3 matrix to (point.x, point.y, 1).
    // The third row is always [0 0 1] so we only compute the first two rows.
    float x = m[0][0] * point.x + m[0][1] * point.y + m[0][2];
    float y = m[1][0] * point.x + m[1][1] * point.y + m[1][2];

    return Vec2(x, y);
}