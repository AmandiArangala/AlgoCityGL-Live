#pragma once

#include "PixelBuffer.h"
#include "CityArea.h"

class Renderer {
public:
    void initialize();
    void renderBackground();

    void renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm);
    void renderCityArea(const CityArea& area, bool xrayMode, int selectedLineAlgorithm);

private:
    PixelBuffer pixelBuffer;

    void buildDay2PixelScene(int selectedLineAlgorithm);
    void buildCityPixelScene(const CityArea& area, int selectedLineAlgorithm, bool xrayMode);

    void drawPixelBuffer(bool xrayMode);
};