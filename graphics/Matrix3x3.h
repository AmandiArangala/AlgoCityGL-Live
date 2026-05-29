#pragma once

#include "CityArea.h"

class Matrix3x3 {
public:
    float m[3][3];

    Matrix3x3();

    static Matrix3x3 identity();
    static Matrix3x3 translation(float tx, float ty);
    static Matrix3x3 rotation(float angleDegrees);
    static Matrix3x3 scaling(float sx, float sy);

    Matrix3x3 operator*(const Matrix3x3& other) const;
    Vec2 transformPoint(const Vec2& point) const;
};