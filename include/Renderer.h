#pragma once

#include "PixelBuffer.h"

class Renderer {
public:
    void initialize();
    void renderBackground();

    void renderDay2TestScene(bool xrayMode, int selectedLineAlgorithm);

private:
    PixelBuffer pixelBuffer;

    void buildDay2PixelScene(int selectedLineAlgorithm);
    void drawPixelBuffer(bool xrayMode);
};