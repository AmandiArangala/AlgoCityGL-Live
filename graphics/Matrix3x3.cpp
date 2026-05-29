#include "Matrix3x3.h"
#include <cmath>

Matrix3x3::Matrix3x3() {
    m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f;
    m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f;
}

Matrix3x3 Matrix3x3::identity() {
    Matrix3x3 result;

    result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f;
    result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f;
    result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f;

    return result;
}

Matrix3x3 Matrix3x3::translation(float tx, float ty) {
    Matrix3x3 result = Matrix3x3::identity();

    result.m[0][2] = tx;
    result.m[1][2] = ty;

    return result;
}

Matrix3x3 Matrix3x3::rotation(float angleDegrees) {
    Matrix3x3 result = Matrix3x3::identity();

    float radians = angleDegrees * 3.14159265f / 180.0f;
    float c = std::cos(radians);
    float s = std::sin(radians);

    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;

    return result;
}

Matrix3x3 Matrix3x3::scaling(float sx, float sy) {
    Matrix3x3 result = Matrix3x3::identity();

    result.m[0][0] = sx;
    result.m[1][1] = sy;

    return result;
}

Matrix3x3 Matrix3x3::operator*(const Matrix3x3& other) const {
    Matrix3x3 result;

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
    float x = m[0][0] * point.x + m[0][1] * point.y + m[0][2];
    float y = m[1][0] * point.x + m[1][1] * point.y + m[1][2];

    return Vec2(x, y);
}