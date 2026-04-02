#pragma once

#include "Fusion/Math/Vec2.h"

namespace Fusion
{

    // ----------------------------------------------------------------
    // FVec4 — 4D float vector
    // Memory layout matches CE::Vec4: [x, y, z, w] (16 bytes)
    // ----------------------------------------------------------------
    struct FVec4
    {
        union
        {
            struct { float x, y, z, w; };
            struct { FVec2 min, max; };
            struct { float left, top, right, bottom; };
            float xyzw[4];
        };

        static constexpr FVec4 Zero() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
        static constexpr FVec4 One()  { return { 1.0f, 1.0f, 1.0f, 1.0f }; }

        constexpr FVec4()                                       : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        constexpr FVec4(float scalar)                           : x(scalar), y(scalar), z(scalar), w(scalar) {}
        constexpr FVec4(float x, float y)                      : x(x), y(y), z(0.0f), w(0.0f) {}
        constexpr FVec4(float x, float y, float z)             : x(x), y(y), z(z), w(0.0f) {}
        constexpr FVec4(float x, float y, float z, float w)    : x(x), y(y), z(z), w(w) {}
        constexpr FVec4(FVec2 xy, FVec2 zw)                    : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

        // ----------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------

        float& operator[](size_t index)
        {
            FUSION_ASSERT(index < 4, "FVec4: index out of bounds");
            return xyzw[index];
        }

        float operator[](size_t index) const
        {
            FUSION_ASSERT(index < 4, "FVec4: index out of bounds");
            return xyzw[index];
        }

        // ----------------------------------------------------------------
        // Arithmetic
        // ----------------------------------------------------------------

        constexpr FVec4 operator+(const FVec4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        constexpr FVec4 operator-(const FVec4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        constexpr FVec4 operator*(const FVec4& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
        constexpr FVec4 operator*(float scalar)     const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        constexpr FVec4 operator/(float scalar)     const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }
        constexpr FVec4 operator+()                 const { return *this; }
        constexpr FVec4 operator-()                 const { return { -x, -y, -z, -w }; }

        constexpr FVec4& operator+=(const FVec4& rhs) { *this = *this + rhs; return *this; }
        constexpr FVec4& operator-=(const FVec4& rhs) { *this = *this - rhs; return *this; }
        constexpr FVec4& operator*=(const FVec4& rhs) { *this = *this * rhs; return *this; }
        constexpr FVec4& operator*=(float scalar)     { *this = *this * scalar; return *this; }
        constexpr FVec4& operator/=(float scalar)     { *this = *this / scalar; return *this; }

        bool operator==(const FVec4& rhs) const { 
        	return FMath::ApproxEquals(x, rhs.x) && 
	            FMath::ApproxEquals(y, rhs.y) && 
	            FMath::ApproxEquals(z, rhs.z) && 
	            FMath::ApproxEquals(w, rhs.w); 
        }

        bool operator!=(const FVec4& rhs) const { return !(*this == rhs); }

        // ----------------------------------------------------------------
        // Vector math
        // ----------------------------------------------------------------

        constexpr float GetSqrMagnitude() const { return x * x + y * y + z * z + w * w; }
        float           GetMagnitude()    const { return std::sqrt(GetSqrMagnitude()); }

        FVec4 GetNormalized() const
        {
            float mag = GetMagnitude();
            return mag > 0.0f ? (*this / mag) : FVec4{};
        }

        static constexpr float Dot(FVec4 a, FVec4 b)         { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
        static constexpr float SqrDistance(FVec4 a, FVec4 b) { return (b - a).GetSqrMagnitude(); }
        static           float Distance(FVec4 a, FVec4 b)    { return (b - a).GetMagnitude(); }

        static constexpr FVec4 Min(FVec4 a, FVec4 b)
        {
            return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y,
                     a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w };
        }

        static constexpr FVec4 Max(FVec4 a, FVec4 b)
        {
            return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y,
                     a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w };
        }

        static constexpr FVec4 Lerp(FVec4 from, FVec4 to, float t)
        {
            return from + (to - from) * t;
        }
    };

    inline constexpr FVec4 operator*(float scalar, FVec4 v) { return v * scalar; }


    // ----------------------------------------------------------------
    // FVec4i — 4D integer vector
    // Memory layout matches CE::Vec4i: [x, y, z, w] (16 bytes)
    // ----------------------------------------------------------------
    struct FVec4i
    {
        union
        {
            struct { int32_t x, y, z, w; };
            int32_t xyzw[4];
        };

        constexpr FVec4i()                                              : x(0), y(0), z(0), w(0) {}
        constexpr FVec4i(int32_t scalar)                                : x(scalar), y(scalar), z(scalar), w(scalar) {}
        constexpr FVec4i(int32_t x, int32_t y)                         : x(x), y(y), z(0), w(0) {}
        constexpr FVec4i(int32_t x, int32_t y, int32_t z)              : x(x), y(y), z(z), w(0) {}
        constexpr FVec4i(int32_t x, int32_t y, int32_t z, int32_t w)   : x(x), y(y), z(z), w(w) {}

        explicit constexpr FVec4i(FVec4 v)
            : x(static_cast<int32_t>(v.x)), y(static_cast<int32_t>(v.y)),
              z(static_cast<int32_t>(v.z)), w(static_cast<int32_t>(v.w)) {}

        explicit constexpr operator FVec4() const
        {
            return { static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(z), static_cast<float>(w) };
        }

        // ----------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------

        int32_t& operator[](size_t index)
        {
            FUSION_ASSERT(index < 4, "FVec4i: index out of bounds");
            return xyzw[index];
        }

        int32_t operator[](size_t index) const
        {
            FUSION_ASSERT(index < 4, "FVec4i: index out of bounds");
            return xyzw[index];
        }

        // ----------------------------------------------------------------
        // Arithmetic
        // ----------------------------------------------------------------

        constexpr FVec4i operator+(const FVec4i& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
        constexpr FVec4i operator-(const FVec4i& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
        constexpr FVec4i operator*(const FVec4i& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
        constexpr FVec4i operator*(int32_t scalar)    const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
        constexpr FVec4i operator/(int32_t scalar)    const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }
        constexpr FVec4i operator+()                  const { return *this; }
        constexpr FVec4i operator-()                  const { return { -x, -y, -z, -w }; }

        constexpr FVec4i& operator+=(const FVec4i& rhs) { *this = *this + rhs; return *this; }
        constexpr FVec4i& operator-=(const FVec4i& rhs) { *this = *this - rhs; return *this; }
        constexpr FVec4i& operator*=(const FVec4i& rhs) { *this = *this * rhs; return *this; }
        constexpr FVec4i& operator*=(int32_t scalar)    { *this = *this * scalar; return *this; }
        constexpr FVec4i& operator/=(int32_t scalar)    { *this = *this / scalar; return *this; }

        constexpr bool operator==(const FVec4i& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
        constexpr bool operator!=(const FVec4i& rhs) const { return !(*this == rhs); }

        // ----------------------------------------------------------------
        // Utility
        // ----------------------------------------------------------------

        static constexpr FVec4i Min(FVec4i a, FVec4i b)
        {
            return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y,
                     a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w };
        }

        static constexpr FVec4i Max(FVec4i a, FVec4i b)
        {
            return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y,
                     a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w };
        }

        static constexpr FVec4i Zero() { return { 0, 0, 0, 0 }; }
        static constexpr FVec4i One()  { return { 1, 1, 1, 1 }; }
    };

    inline constexpr FVec4i operator*(int32_t scalar, FVec4i v) { return v * scalar; }

    using FMargin = FVec4;

} // namespace Fusion
