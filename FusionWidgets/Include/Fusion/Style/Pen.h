#pragma once

namespace Fusion
{
	enum class EPenStyle : u32
	{
		None,
		Solid,
		Dashed, // Uses dashLength and dashGap
		Dotted  // Uses dashLength and dashGap (typically dashLength == thickness for round dots)
	};
	FUSION_ENUM_CLASS(EPenStyle);

	enum class EGradientSpace : u32
	{
		ArcLength,  // Gradient travels along stroke arc-length (default)
		WorldSpace  // Gradient projects across the shape's bounding box by angle
	};
	FUSION_ENUM_CLASS(EGradientSpace);

    class FUSIONWIDGETS_API FPen final
    {
    public:

		FPen() = default;

		// Implicit ctors
		FPen(const FColor& color, f32 thickness = 1.0f, EPenStyle style = EPenStyle::Solid);
		FPen(const FGradient& gradient, f32 thickness = 1.0f, EPenStyle style = EPenStyle::Solid, const FColor& tint = FColors::White);

		// Named factories
		static FPen Solid(const FColor& color, f32 thickness = 1.0f);
		static FPen Dashed(const FColor& color, f32 thickness = 1.0f, f32 dashLength = 5.0f, f32 dashGap = 5.0f);
		static FPen Dotted(const FColor& color, f32 thickness = 1.0f, f32 dashGap = 5.0f);
		static FPen Gradient(const FGradient& gradient, f32 thickness = 1.0f, const FColor& tint = FColors::White);

		// - Getters -

		const FColor&    GetColor()          const { return m_Color; }
		const FGradient& GetGradient()       const { return m_Gradient; }
		EPenStyle        GetStyle()          const { return m_Style; }
		f32              GetThickness()      const { return m_Thickness; }
		f32              GetDashLength()     const { return m_DashLength; }
		f32              GetDashGap()        const { return m_DashGap; }
		f32              GetGradientOffset() const { return m_GradientOffset; }
		f32              GetDashPhase()      const { return m_DashPhase; }
		EGradientSpace   GetGradientSpace()  const { return m_GradientSpace; }

		bool HasGradient() const { return m_Gradient.IsValid(); }

		bool IsValidPen() const
		{
			return (HasGradient() || m_Color.a > 0.001f) && m_Thickness > 0.01f;
		}

		// - Fluent setters -

		FPen& Color(const FColor& color)       { m_Color = color;           return *this; }
		FPen& Thickness(f32 thickness)         { m_Thickness = thickness;   return *this; }
		FPen& Style(EPenStyle style)           { m_Style = style;           return *this; }
		FPen& DashLength(f32 length)           { m_DashLength = length;     return *this; }
		FPen& DashGap(f32 gap)                 { m_DashGap = gap;           return *this; }
		FPen& GradientOffset(f32 offset)            { m_GradientOffset = offset;  return *this; }
		FPen& DashPhase(f32 phase)                  { m_DashPhase = phase;        return *this; }
		FPen& GradientSpace(EGradientSpace space)   { m_GradientSpace = space;    return *this; }

		// Computes the initial dashOffset and inDash state from dashPhase.
		// Uses dashLength as the "on" length for Dashed, and 2pt for Dotted.
		void InitDashState(f32& outDashOffset, bool& outInDash) const
		{
			constexpr f32 dotLen = 2.0f;
			const f32 onLen  = (m_Style == EPenStyle::Dotted) ? dotLen : m_DashLength;
			const f32 period = onLen + m_DashGap;
			const f32 phase  = period > 0.0f ? fmod(m_DashPhase, period) : 0.0f;
			outInDash    = phase < onLen;
			outDashOffset = outInDash ? phase : phase - onLen;
		}

    private:

		FGradient m_Gradient;
		FColor    m_Color;

		f32 m_Thickness      = 0.0f;
		f32 m_DashLength     = 5.0f;
		f32 m_DashGap        = 5.0f;
		f32 m_GradientOffset = 0.0f;
		f32 m_DashPhase      = 0.0f;

		EPenStyle      m_Style         = EPenStyle::None;
		EGradientSpace m_GradientSpace = EGradientSpace::ArcLength;
    };

} // namespace Fusion
