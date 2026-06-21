/**
 * @file Color.h
 * @brief Defines the Color struct — an RGBA floating-point colour value.
 *
 * All colour channels use the range [0.0, 1.0] where:
 *   0.0 = no contribution (black / fully transparent)
 *   1.0 = full contribution (full brightness / fully opaque)
 *
 * Colors are used by:
 *  - PixelBuffer::putPixel() — to store pixel colours for algorithm visualisation.
 *  - ImGui draw-list calls (ImDrawList::AddLine, AddRectFilled, etc.) — converted
 *    to ImVec4 or ImU32 inside the Renderer as needed.
 *
 * Default construction produces opaque white: Color(1, 1, 1, 1).
 */

#pragma once

/**
 * @brief RGBA floating-point colour (each channel in [0.0 … 1.0]).
 *
 * Examples:
 *   Color(1, 0, 0)       → opaque red
 *   Color(0, 1, 0, 0.5f) → semi-transparent green
 *   Color()              → opaque white (default)
 */
struct Color {
    float r; ///< Red channel   [0.0 … 1.0].
    float g; ///< Green channel [0.0 … 1.0].
    float b; ///< Blue channel  [0.0 … 1.0].
    float a; ///< Alpha channel [0.0 = transparent … 1.0 = opaque].

    /** @brief Construct a colour; defaults to opaque white (1, 1, 1, 1). */
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
};