#include "Fusion/Widgets.h"

namespace Fusion
{

	f32 FEasingCurve::Evaluate(f32 t) const
	{
		ZoneScoped;

		// Clamp for non-overshoot curves; overshoot curves (Back, Elastic) naturally exceed [0,1]
		t = FMath::Clamp01(t);

		switch (Type)
		{
		default:
		case EEasingType::Linear:
			return t;

			// --- Quad ---
		case EEasingType::EaseInQuad:
			return t * t;
		case EEasingType::EaseOutQuad:
			return t * (2.0f - t);
		case EEasingType::EaseInOutQuad:
			return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

			// --- Cubic ---
		case EEasingType::EaseInCubic:
			return t * t * t;
		case EEasingType::EaseOutCubic:
		{
			f32 t1 = t - 1.0f;
			return t1 * t1 * t1 + 1.0f;
		}
		case EEasingType::EaseInOutCubic:
			return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;

			// --- Quart ---
		case EEasingType::EaseInQuart:
			return t * t * t * t;
		case EEasingType::EaseOutQuart:
		{
			f32 t1 = t - 1.0f;
			return 1.0f - t1 * t1 * t1 * t1;
		}
		case EEasingType::EaseInOutQuart:
		{
			f32 t1 = t - 1.0f;
			return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * t1 * t1 * t1 * t1;
		}

		// --- Sine ---
		case EEasingType::EaseInSine:
			return 1.0f - FMath::Cos(t * FMath::PI * 0.5f);
		case EEasingType::EaseOutSine:
			return FMath::Sin(t * FMath::PI * 0.5f);
		case EEasingType::EaseInOutSine:
			return 0.5f * (1.0f - FMath::Cos(FMath::PI * t));

			// --- Expo ---
		case EEasingType::EaseInExpo:
			return t == 0.0f ? 0.0f : FMath::Pow(2.0f, 10.0f * t - 10.0f);
		case EEasingType::EaseOutExpo:
			return t == 1.0f ? 1.0f : 1.0f - FMath::Pow(2.0f, -10.0f * t);
		case EEasingType::EaseInOutExpo:
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			return t < 0.5f
				? FMath::Pow(2.0f, 20.0f * t - 10.0f) * 0.5f
				: (2.0f - FMath::Pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;

			// --- Circ ---
		case EEasingType::EaseInCirc:
			return 1.0f - FMath::Sqrt(1.0f - t * t);
		case EEasingType::EaseOutCirc:
		{
			f32 t1 = t - 1.0f;
			return FMath::Sqrt(1.0f - t1 * t1);
		}
		case EEasingType::EaseInOutCirc:
		{
			f32 t1 = 2.0f * t - 2.0f;
			return t < 0.5f
				? 0.5f * (1.0f - FMath::Sqrt(1.0f - 4.0f * t * t))
				: 0.5f * (FMath::Sqrt(1.0f - t1 * t1) + 1.0f);
		}

		// --- Back (overshoots slightly past 1) ---
		case EEasingType::EaseInBack:
		{
			constexpr f32 c = 1.70158f;
			return t * t * ((c + 1.0f) * t - c);
		}
		case EEasingType::EaseOutBack:
		{
			constexpr f32 c = 1.70158f;
			f32 t1 = t - 1.0f;
			return t1 * t1 * ((c + 1.0f) * t1 + c) + 1.0f;
		}
		case EEasingType::EaseInOutBack:
		{
			constexpr f32 c1 = 1.70158f;
			constexpr f32 c2 = c1 * 1.525f;
			f32 t1 = 2.0f * t - 2.0f;
			return t < 0.5f
				? (2.0f * t * 2.0f * t * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
				: (t1 * t1 * ((c2 + 1.0f) * t1 + c2) + 2.0f) * 0.5f;
		}

		// --- Bounce ---
		case EEasingType::EaseOutBounce:
		{
			constexpr f32 n = 7.5625f;
			constexpr f32 d = 2.75f;
			if (t < 1.0f / d)
				return n * t * t;
			else if (t < 2.0f / d)
			{
				t -= 1.5f / d;
				return n * t * t + 0.75f;
			}
			else if (t < 2.5f / d)
			{
				t -= 2.25f / d;
				return n * t * t + 0.9375f;
			}
			else
			{
				t -= 2.625f / d;
				return n * t * t + 0.984375f;
			}
		}
		case EEasingType::EaseInBounce:
		{
			// EaseIn = 1 - EaseOut(1 - t)
			f32 t1 = 1.0f - t;
			FEasingCurve out(EEasingType::EaseOutBounce);
			return 1.0f - out.Evaluate(t1);
		}
		case EEasingType::EaseInOutBounce:
		{
			FEasingCurve out(EEasingType::EaseOutBounce);
			return t < 0.5f
				? (1.0f - out.Evaluate(1.0f - 2.0f * t)) * 0.5f
				: (1.0f + out.Evaluate(2.0f * t - 1.0f)) * 0.5f;
		}

		// --- Elastic (overshoots, spring-like) ---
		case EEasingType::EaseInElastic:
		{
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			constexpr f32 c = (2.0f * FMath::PI) / 3.0f;
			return -FMath::Pow(2.0f, 10.0f * t - 10.0f) * FMath::Sin((t * 10.0f - 10.75f) * c);
		}
		case EEasingType::EaseOutElastic:
		{
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			constexpr f32 c = (2.0f * FMath::PI) / 3.0f;
			return FMath::Pow(2.0f, -10.0f * t) * FMath::Sin((t * 10.0f - 0.75f) * c) + 1.0f;
		}
		case EEasingType::EaseInOutElastic:
		{
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			constexpr f32 c = (2.0f * FMath::PI) / 4.5f;
			return t < 0.5f
				? -(FMath::Pow(2.0f, 20.0f * t - 10.0f) * FMath::Sin((20.0f * t - 11.125f) * c)) * 0.5f
				: (FMath::Pow(2.0f, -20.0f * t + 10.0f) * FMath::Sin((20.0f * t - 11.125f) * c)) * 0.5f + 1.0f;
		}

		// --- Cubic Bezier ---
		case EEasingType::CubicBezier:
		{
			// Newton-Raphson solve for the parameter s such that BezierX(s) == t,
			// then return BezierY(s). P0=(0,0), P3=(1,1); P1=controlPoint1, P2=controlPoint2.
			auto BezierCoord = [](f32 p1, f32 p2, f32 s) -> f32
				{
					// Cubic Bernstein: 3*(1-s)^2*s*p1 + 3*(1-s)*s^2*p2 + s^3
					f32 s1 = 1.0f - s;
					return 3.0f * s1 * s1 * s * p1 + 3.0f * s1 * s * s * p2 + s * s * s;
				};
			auto BezierCoordDerivative = [](f32 p1, f32 p2, f32 s) -> f32
				{
					f32 s1 = 1.0f - s;
					return 3.0f * s1 * s1 * p1 + 6.0f * s1 * s * (p2 - p1) + 3.0f * s * s * (1.0f - p2);
				};

			// Solve BezierX(s) = t via Newton-Raphson (8 iterations is sufficient)
			f32 s = t;
			for (int i = 0; i < 8; ++i)
			{
				f32 bx = BezierCoord(ControlPoint1.x, ControlPoint2.x, s) - t;
				f32 bxd = BezierCoordDerivative(ControlPoint1.x, ControlPoint2.x, s);
				if (FMath::Abs(bxd) < 1e-6f) break;
				s -= bx / bxd;
				s = FMath::Clamp01(s);
			}

			return BezierCoord(ControlPoint1.y, ControlPoint2.y, s);
		}
		}
	}
    
} // namespace Fusion
