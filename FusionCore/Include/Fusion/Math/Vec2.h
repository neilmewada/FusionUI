#pragma once

#include <cstdint>
#include <cmath>

#include "Fusion/Misc/Assert.h"

namespace Fusion
{

    // ----------------------------------------------------------------
    // FVec2 — 2D float vector
    // Memory layout matches CE::Vec2: [x, y] (8 bytes)
    // ----------------------------------------------------------------
    struct FVec2
    {
        union
        {
            struct { float x, y; };
            struct { float width, height; };
            struct { float min, max; };
            struct { float left, right; };
            struct { float top, bottom; };
            float xy[2];
        };

        static constexpr FVec2 Zero() { return { 0.0f, 0.0f }; }
        static constexpr FVec2 One()  { return { 1.0f, 1.0f }; }

        constexpr FVec2()                  : x(0.0f), y(0.0f) {}
        constexpr FVec2(float scalar)      : x(scalar), y(scalar) {}
        constexpr FVec2(float x, float y)  : x(x), y(y) {}

        // ----------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------

        float& operator[](size_t index)
        {
            FUSION_ASSERT(index < 2, "FVec2: index out of bounds");
            return xy[index];
        }

        float operator[](size_t index) const
        {
            FUSION_ASSERT(index < 2, "FVec2: index out of bounds");
            return xy[index];
        }

        // ----------------------------------------------------------------
        // Arithmetic
        // ----------------------------------------------------------------

        constexpr FVec2 operator+(const FVec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
        constexpr FVec2 operator-(const FVec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
        constexpr FVec2 operator*(const FVec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
        constexpr FVec2 operator*(float scalar)     const { return { x * scalar, y * scalar }; }
        constexpr FVec2 operator/(float scalar)     const { return { x / scalar, y / scalar }; }
        constexpr FVec2 operator+()                 const { return *this; }
        constexpr FVec2 operator-()                 const { return { -x, -y }; }

        constexpr FVec2& operator+=(const FVec2& rhs) { *this = *this + rhs; return *this; }
        constexpr FVec2& operator-=(const FVec2& rhs) { *this = *this - rhs; return *this; }
        constexpr FVec2& operator*=(const FVec2& rhs) { *this = *this * rhs; return *this; }
        constexpr FVec2& operator*=(float scalar)     { *this = *this * scalar; return *this; }
        constexpr FVec2& operator/=(float scalar)     { *this = *this / scalar; return *this; }

        bool operator==(const FVec2& rhs) const { return FMath::ApproxEquals(x, rhs.x) && FMath::ApproxEquals(y, rhs.y); }
        bool operator!=(const FVec2& rhs) const { return !(*this == rhs); }

        // ----------------------------------------------------------------
        // Vector math
        // ----------------------------------------------------------------

        constexpr float GetSqrMagnitude() const { return x * x + y * y; }
        float           GetMagnitude()    const { return std::sqrt(GetSqrMagnitude()); }

        FVec2 GetNormalized() const
        {
            float mag = GetMagnitude();
            return mag > 0.0f ? (*this / mag) : FVec2{};
        }

        static constexpr float Dot(FVec2 a, FVec2 b)         { return a.x * b.x + a.y * b.y; }
        static constexpr float SqrDistance(FVec2 a, FVec2 b) { return (b - a).GetSqrMagnitude(); }
        static           float Distance(FVec2 a, FVec2 b)    { return (b - a).GetMagnitude(); }

        static constexpr FVec2 Min(FVec2 a, FVec2 b)
        {
            return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y };
        }

        static constexpr FVec2 Max(FVec2 a, FVec2 b)
        {
            return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y };
        }

        static constexpr FVec2 Lerp(FVec2 from, FVec2 to, float t)
        {
            return from + (to - from) * t;
        }
    };

    inline constexpr FVec2 operator*(float scalar, FVec2 v) { return v * scalar; }


    // ----------------------------------------------------------------
    // FVec2i — 2D integer vector
    // Memory layout matches CE::Vec2i: [x, y] (8 bytes)
    // ----------------------------------------------------------------
    struct FVec2i
    {
        union
        {
            struct { int32_t x, y; };
            struct { int32_t width, height; };
            int32_t xy[2];
        };

        constexpr FVec2i()                     : x(0), y(0) {}
        constexpr FVec2i(int32_t scalar)        : x(scalar), y(scalar) {}
        constexpr FVec2i(int32_t x, int32_t y) : x(x), y(y) {}

        explicit constexpr FVec2i(FVec2 v)
            : x(static_cast<int32_t>(v.x)), y(static_cast<int32_t>(v.y)) {}

        explicit constexpr operator FVec2() const
        {
            return { static_cast<float>(x), static_cast<float>(y) };
        }

        // ----------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------

        int32_t& operator[](size_t index)
        {
            FUSION_ASSERT(index < 2, "FVec2i: index out of bounds");
            return xy[index];
        }

        int32_t operator[](size_t index) const
        {
            FUSION_ASSERT(index < 2, "FVec2i: index out of bounds");
            return xy[index];
        }

        // ----------------------------------------------------------------
        // Arithmetic
        // ----------------------------------------------------------------

        constexpr FVec2i operator+(const FVec2i& rhs) const { return { x + rhs.x, y + rhs.y }; }
        constexpr FVec2i operator-(const FVec2i& rhs) const { return { x - rhs.x, y - rhs.y }; }
        constexpr FVec2i operator*(const FVec2i& rhs) const { return { x * rhs.x, y * rhs.y }; }
        constexpr FVec2i operator*(int32_t scalar)    const { return { x * scalar, y * scalar }; }
        constexpr FVec2i operator/(int32_t scalar)    const { return { x / scalar, y / scalar }; }
        constexpr FVec2i operator+()                  const { return *this; }
        constexpr FVec2i operator-()                  const { return { -x, -y }; }

        constexpr FVec2i& operator+=(const FVec2i& rhs) { *this = *this + rhs; return *this; }
        constexpr FVec2i& operator-=(const FVec2i& rhs) { *this = *this - rhs; return *this; }
        constexpr FVec2i& operator*=(const FVec2i& rhs) { *this = *this * rhs; return *this; }
        constexpr FVec2i& operator*=(int32_t scalar)    { *this = *this * scalar; return *this; }
        constexpr FVec2i& operator/=(int32_t scalar)    { *this = *this / scalar; return *this; }

        constexpr bool operator==(const FVec2i& rhs) const { return x == rhs.x && y == rhs.y; }
        constexpr bool operator!=(const FVec2i& rhs) const { return !(*this == rhs); }

        // ----------------------------------------------------------------
        // Utility
        // ----------------------------------------------------------------

        static constexpr FVec2i Min(FVec2i a, FVec2i b)
        {
            return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y };
        }

        static constexpr FVec2i Max(FVec2i a, FVec2i b)
        {
            return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y };
        }

        static constexpr FVec2i Zero() { return { 0, 0 }; }
        static constexpr FVec2i One()  { return { 1, 1 }; }
    };

    inline constexpr FVec2i operator*(int32_t scalar, FVec2i v) { return v * scalar; }

} // namespace Fusion

