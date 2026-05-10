#pragma once

#include <format>

#include "Fusion/Math/Vec2.h"

namespace Fusion
{

    // ----------------------------------------------------------------
    // FRect — axis-aligned rectangle
    // Memory layout: [left, top, right, bottom] (16 bytes)
    // Equivalently: [min.x, min.y, max.x, max.y]
    // ----------------------------------------------------------------
    struct FRect
    {
        float left, top, right, bottom;

        constexpr FRect() : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}

        constexpr FRect(float left, float top, float right, float bottom)
            : left(left), top(top), right(right), bottom(bottom) {}

        constexpr FRect(FVec2 min, FVec2 max)
            : left(min.x), top(min.y), right(max.x), bottom(max.y) {}

        // Construct from a position and a size (width/height), not two corners
        static constexpr FRect FromSize(FVec2 position, FVec2 size)
        {
            return { position, position + size };
        }

        static constexpr FRect FromSize(float x, float y, float w, float h)
        {
            return { x, y, x + w, y + h };
        }

        // ----------------------------------------------------------------
        // Properties
        // ----------------------------------------------------------------

        constexpr FVec2 GetSize()        const { return { right - left, bottom - top }; }
        constexpr float GetWidth()       const { return right - left; }
        constexpr float GetHeight()      const { return bottom - top; }
        constexpr FVec2 GetCenter()      const { return FVec2{ (left + right) * 0.5f, (top + bottom) * 0.5f }; }
        constexpr float GetArea()        const { return GetWidth() * GetHeight(); }
        constexpr i32   GetAreaInt()     const { return (i32)(GetWidth() * GetHeight()); }
        constexpr bool  IsEmpty()        const { return left >= right || top >= bottom; }
        constexpr FVec2 GetTopLeft()     const { return FVec2(left, top); }
        constexpr FVec2 GetBottomRight() const { return FVec2(right, bottom); }

        // ----------------------------------------------------------------
        // Containment / overlap
        // ----------------------------------------------------------------

        constexpr bool Contains(FVec2 point) const
        {
            return point.x >= left && point.y >= top
                && point.x <= right && point.y <= bottom;
        }

        constexpr bool Overlaps(const FRect& other) const
        {
            return !(right < other.left || left > other.right
                  || bottom < other.top || top > other.bottom);
        }

        // ----------------------------------------------------------------
        // Transformations (all return a new FRect — immutable style)
        // ----------------------------------------------------------------

        constexpr FRect Translate(FVec2 offset) const
        {
            return { left + offset.x, top + offset.y, right + offset.x, bottom + offset.y };
        }

        constexpr FRect Scale(FVec2 scale) const
        {
            FVec2 center = GetCenter();
            FVec2 half   = GetSize() * 0.5f;
            FVec2 mn = center - half * scale;
            FVec2 mx = center + half * scale;
            return { mn.x, mn.y, mx.x, mx.y };
        }

        constexpr FRect Scale(float scale) const
        {
            return Scale(FVec2(scale));
        }

        // Expand (positive) or shrink (negative) uniformly on all sides
        constexpr FRect Expand(float amount) const
        {
            return { left - amount, top - amount, right + amount, bottom + amount };
        }

        // Expand (positive) or shrink (negative) uniformly on all sides
        constexpr FRect Expand(FVec2 amount) const
        {
            return { left - amount.x, top - amount.y, right + amount.x, bottom + amount.y };
        }

        constexpr FRect Encapsulate(FVec2 point) const
        {
            return { FVec2::Min({left, top}, point), FVec2::Max({right, bottom}, point) };
        }

        constexpr FRect Encapsulate(const FRect& other) const
        {
            return { FVec2::Min({left, top}, {other.left, other.top}),
                     FVec2::Max({right, bottom}, {other.right, other.bottom}) };
        }

        // ----------------------------------------------------------------
        // Set operations
        // ----------------------------------------------------------------

        // Smallest rect that contains both a and b
        static constexpr FRect Union(const FRect& a, const FRect& b)
        {
            if (a.IsEmpty()) return b;
            if (b.IsEmpty()) return a;
            return { FVec2::Min({a.left, a.top}, {b.left, b.top}),
                     FVec2::Max({a.right, a.bottom}, {b.right, b.bottom}) };
        }

        // Overlapping region of a and b, or an empty rect if they don't overlap
        static constexpr FRect Intersection(const FRect& a, const FRect& b)
        {
            if (!a.Overlaps(b)) return {};
            return { FVec2::Max({a.left, a.top}, {b.left, b.top}),
                     FVec2::Min({a.right, a.bottom}, {b.right, b.bottom}) };
        }

        // ----------------------------------------------------------------
        // Operators
        // ----------------------------------------------------------------

        bool operator==(const FRect& rhs) const
        {
            return FVec2{left, top} == FVec2{rhs.left, rhs.top} &&
                   FVec2{right, bottom} == FVec2{rhs.right, rhs.bottom};
        }
        bool operator!=(const FRect& rhs) const { return !(*this == rhs); }

        constexpr FRect operator+(const FRect& rhs) const
        {
            return { left + rhs.left, top + rhs.top, right + rhs.right, bottom + rhs.bottom };
        }

        constexpr FRect operator-(const FRect& rhs) const
        {
            return { left - rhs.left, top - rhs.top, right - rhs.right, bottom - rhs.bottom };
        }

        constexpr FRect& operator+=(const FRect& rhs)
        {
            left += rhs.left; top += rhs.top; right += rhs.right; bottom += rhs.bottom;
            return *this;
        }

        constexpr FRect& operator-=(const FRect& rhs)
        {
            left -= rhs.left; top -= rhs.top; right -= rhs.right; bottom -= rhs.bottom;
            return *this;
        }
    };

} // namespace Fusion

template<>
struct std::formatter<Fusion::FRect> : std::formatter<float>
{
    auto format(const Fusion::FRect& r, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", r.left, r.top, r.right, r.bottom);
    }
};
