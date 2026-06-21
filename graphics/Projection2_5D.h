/**
 * @file Projection2_5D.h
 * @brief Declares the isometric (2.5D) projection used for city rendering.
 *
 * 2.5D (dimetric / isometric) projection gives a 3D appearance to a 2D scene
 * by slanting the flat (x,y) world coordinates into a pseudo-3D view.
 *
 * Formulas used (see Projection2_5D.cpp for constants):
 *   isoX = (worldX - worldY) * 0.75 + screenCentreX
 *   isoY = (worldX + worldY) * 0.375 + topPadding
 *
 * Buildings are then shifted vertically by their height:
 *   topY = isoY - height * 0.6
 */
#pragma once

#include "CityArea.h"

class Projection2_5D {
public:
    /**
     * @brief Projects a flat world point into 2.5D isometric screen space.
     * @param point  World (x, y) coordinate.
     * @return       Corresponding screen (isoX, isoY) coordinate.
     */
    static Vec2 projectPoint(const Vec2& point);

    /**
     * @brief Shifts an already-projected point upward on screen by a scaled height.
     *
     * Used to compute the \"top\" of a building face in isometric view:
     *   topY = point.y - height * 0.6
     *
     * @param point   Projected base point.
     * @param height  Building height in world units.
     * @return        The projected top-of-building point.
     */
    static Vec2 shiftUp(const Vec2& point, float height);
};