#pragma once

#include <cstdint>

#include "Fusion/Misc/Assert.h"

namespace Fusion
{

    // ----------------------------------------------------------------
    // FColor — RGBA float color (components in [0, 1])
    //
    // Memory layout: stored as ABGR in memory
    //   struct { float a, b, g, r; }  →  bytes: [a][b][g][r]
    // Public API is RGBA-ordered (constructor, operator[], ToU32).
    // ----------------------------------------------------------------
    struct FColor
    {
        union
        {
            struct { float a, b, g, r; }; // stored ABGR in memory
            float abgr[4];
        };

        // ----------------------------------------------------------------
        // Construction
        // ----------------------------------------------------------------

        constexpr FColor() : a(0.0f), b(0.0f), g(0.0f), r(0.0f) {}

        constexpr FColor(float r, float g, float b, float a = 1.0f)
            : a(a), b(b), g(g), r(r) {}

        // From 8-bit RGBA channels [0, 255]
        static constexpr FColor RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
        }

        static constexpr FColor RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            return RGBA8(r, g, b, a);
        }

        // From a 24-bit RGB hex value (e.g. 0xFF8800)
        static constexpr FColor RGBHex(uint32_t hex)
        {
            return RGBA8(
                static_cast<uint8_t>(hex >> 16),
                static_cast<uint8_t>(hex >> 8),
                static_cast<uint8_t>(hex)
            );
        }

        // From a 32-bit RGBA hex value (e.g. 0xFF8800FF)
        static constexpr FColor RGBAHex(uint32_t hex)
        {
            return RGBA8(
                static_cast<uint8_t>(hex >> 24),
                static_cast<uint8_t>(hex >> 16),
                static_cast<uint8_t>(hex >> 8),
                static_cast<uint8_t>(hex)
            );
        }

        // From HSV (h: [0, 360), s: [0, 1], v: [0, 1]) — defined in Color.cpp
        static FColor HSV(float h, float s, float v);

        // ----------------------------------------------------------------
        // Element access — RGBA indexed order: [0]=r, [1]=g, [2]=b, [3]=a
        // ----------------------------------------------------------------

        float operator[](size_t index) const
        {
            FUSION_ASSERT(index < 4, "FColor: index out of bounds");
            switch (index)
            {
                case 0:  return r;
                case 1:  return g;
                case 2:  return b;
                default: return a;
            }
        }

        // ----------------------------------------------------------------
        // Conversion
        // ----------------------------------------------------------------

        // Pack to 32-bit unsigned: r | (g<<8) | (b<<16) | (a<<24)
        constexpr uint32_t ToU32() const
        {
            auto pack = [](float v) -> uint32_t
            {
                int i = static_cast<int>(v * 255.0f);
                return static_cast<uint32_t>(i < 0 ? 0 : i > 255 ? 255 : i);
            };
            return  pack(r)         |
                   (pack(g) << 8)   |
                   (pack(b) << 16)  |
                   (pack(a) << 24);
        }

        constexpr FColor WithAlpha(float alpha) const { return { r, g, b, alpha }; }

        // ----------------------------------------------------------------
        // Arithmetic
        // ----------------------------------------------------------------

        constexpr FColor operator*(float value) const { return { r * value, g * value, b * value, a * value }; }
        constexpr FColor operator/(float value) const { return { r / value, g / value, b / value, a / value }; }

        constexpr FColor& operator*=(float value) { *this = *this * value; return *this; }
        constexpr FColor& operator/=(float value) { *this = *this / value; return *this; }

        constexpr bool operator==(const FColor& rhs) const
        {
            return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
        }

        constexpr bool operator!=(const FColor& rhs) const { return !(*this == rhs); }

        // ----------------------------------------------------------------
        // Interpolation
        // ----------------------------------------------------------------

        static constexpr FColor Lerp(FColor from, FColor to, float t)
        {
            return {
                from.r + (to.r - from.r) * t,
                from.g + (to.g - from.g) * t,
                from.b + (to.b - from.b) * t,
                from.a + (to.a - from.a) * t
            };
        }
    };

    inline constexpr FColor operator*(float value, FColor c) { return c * value; }

    // ----------------------------------------------------------------
    // Predefined colors
    // ----------------------------------------------------------------
    namespace FColors
    {
        // Primary
        inline constexpr FColor Red     = { 1.0f, 0.0f, 0.0f };
        inline constexpr FColor Green   = { 0.0f, 1.0f, 0.0f };
        inline constexpr FColor Blue    = { 0.0f, 0.0f, 1.0f };

        // Secondary
        inline constexpr FColor Yellow  = { 1.0f, 1.0f, 0.0f };
        inline constexpr FColor Cyan    = { 0.0f, 1.0f, 1.0f };
        inline constexpr FColor Magenta = { 1.0f, 0.0f, 1.0f };

        // Greyscale
        inline constexpr FColor White     = { 1.0f,  1.0f,  1.0f  };
        inline constexpr FColor Black     = { 0.0f,  0.0f,  0.0f  };
        inline constexpr FColor Gray      = { 0.5f,  0.5f,  0.5f  };
        inline constexpr FColor LightGray = { 0.75f, 0.75f, 0.75f };
        inline constexpr FColor DarkGray  = { 0.25f, 0.25f, 0.25f };
        inline constexpr FColor Clear     = { 0.0f,  0.0f,  0.0f,  0.0f };

        // Extra
        inline constexpr FColor Orange  = { 1.0f,  0.6f,  0.0f  };
        inline constexpr FColor Purple  = { 0.5f,  0.0f,  0.5f  };
        inline constexpr FColor Pink    = { 1.0f,  0.4f,  0.7f  };
        inline constexpr FColor Lime    = { 0.5f,  1.0f,  0.0f  };
        inline constexpr FColor Teal    = { 0.0f,  0.5f,  0.5f  };
        inline constexpr FColor Olive   = { 0.5f,  0.5f,  0.0f  };
        inline constexpr FColor Maroon  = { 0.5f,  0.0f,  0.0f  };
        inline constexpr FColor Navy    = { 0.0f,  0.0f,  0.5f  };
        inline constexpr FColor Gold    = { 1.0f,  0.84f, 0.0f  };
        inline constexpr FColor SkyBlue = { 0.53f, 0.81f, 0.92f };
    }

} // namespace Fusion
