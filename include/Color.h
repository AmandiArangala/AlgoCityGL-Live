/**
 * @file Color.h
 * @brief Defines the Color struct — an RGBA floating-point colour value.
 *
 * All channels use the normalised range [0.0, 1.0]:
 *   - 0.0 = no contribution (black / fully transparent)
 *   - 1.0 = full contribution (white / fully opaque)
 *
 * This struct is used by PixelBuffer, LineAlgorithms, CircleAlgorithms, and
 * FillAlgorithms to colour individual pixels written during rasterisation.
 */
#pragma once

struct Color {
    float r; ///< Red   channel  [0.0, 1.0]
    float g; ///< Green channel  [0.0, 1.0]
    float b; ///< Blue  channel  [0.0, 1.0]
    float a; ///< Alpha channel  [0.0, 1.0] (1.0 = fully opaque)

    /**
     * @brief Constructs a colour with the given RGBA components.
     * @param r Red   (default 1.0 — full red, produces white when r=g=b=1).
     * @param g Green (default 1.0).
     * @param b Blue  (default 1.0).
     * @param a Alpha (default 1.0 — fully opaque).
     */
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
};