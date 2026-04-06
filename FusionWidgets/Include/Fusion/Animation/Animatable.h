#pragma once

namespace Fusion
{

    template<typename T>
    struct FAnimatable
    {
        static constexpr bool Supported = requires (T a, T b) { a + b; };

        static T Lerp(const T& a, const T& b, f32 t) { return a + (b - a) * FMath::Clamp01(t); }

        static T LerpUnclamped(const T& a, const T& b, f32 t) { return a + (b - a) * t; }

        // Identity/zero value — needed for spring initial velocity
        static T Identity() { return {}; }

        // Spring arithmetic helpers
        static T Add(const T& a, const T& b) { return a + b; }
        static T Scale(const T& a, f32 s) { return a * s; }

        static f32 SquaredMagnitude(const T& v)
        {
            if constexpr (requires { v.GetSqrMagnitude(); })
                return v.GetSqrMagnitude();
            else
                return static_cast<f32>(v * v);
        }
    };
    
} // namespace Fusion
