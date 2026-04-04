#include "Fusion/Widgets.h"

namespace Fusion
{
	// - Implicit ctors -

	FPen::FPen(const FColor& color, f32 thickness, EPenStyle style)
		: m_Color(color), m_Thickness(thickness), m_Style(style)
	{
	}

	FPen::FPen(const FGradient& gradient, f32 thickness, EPenStyle style, const FColor& tint)
		: m_Gradient(gradient), m_Color(tint), m_Thickness(thickness), m_Style(style)
	{
	}

	// - Named factories -

	FPen FPen::Solid(const FColor& color, f32 thickness)
	{
		return FPen(color, thickness, EPenStyle::Solid);
	}

	FPen FPen::Dashed(const FColor& color, f32 thickness, f32 dashLength, f32 dashGap)
	{
		FPen p(color, thickness, EPenStyle::Dashed);
		p.m_DashLength = dashLength;
		p.m_DashGap    = dashGap;
		return p;
	}

	FPen FPen::Dotted(const FColor& color, f32 thickness, f32 dashGap)
	{
		FPen p(color, thickness, EPenStyle::Dotted);
		p.m_DashGap = dashGap;
		return p;
	}

	FPen FPen::Gradient(const FGradient& gradient, f32 thickness, const FColor& tint)
	{
		return FPen(gradient, thickness, EPenStyle::Solid, tint);
	}

} // namespace Fusion
