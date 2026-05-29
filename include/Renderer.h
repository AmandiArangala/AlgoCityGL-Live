#pragma once

#include "PixelBuffer.h"
#include "CityArea.h"

class Renderer {
public:
    void initialize();
    void renderBackground();

    void renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm);

    void renderCityArea(
        const CityArea& area,
        bool xrayMode,
        int selectedLineAlgorithm,
        bool isometricMode
    );

private:
    PixelBuffer pixelBuffer;

    void buildDay2PixelScene(int selectedLineAlgorithm);

    void buildCityPixelScene(
        const CityArea& area,
        int selectedLineAlgorithm,
        bool xrayMode,
        bool isometricMode
    );

    Vec2 transformForView(const Vec2& point, bool isometricMode);
    void drawBuildingFills2_5D(const CityArea& area);
    void drawPixelBuffer(bool xrayMode);
};