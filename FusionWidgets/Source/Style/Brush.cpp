#include "Fusion/Widgets.h"

namespace Fusion
{
	FBrush::FBrush(const FColor& color)              : FBrush(Solid(color)) {}
	FBrush::FBrush(const FName& imagePath, const FColor& tint) : FBrush(Image(imagePath, tint)) {}
	FBrush::FBrush(const FGradient& gradient, const FColor& tint) : FBrush(Gradient(gradient, tint)) {}

	FBrush FBrush::Solid(const FColor& color)
	{
		FBrush b;
		b.m_BrushStyle = EBrushStyle::SolidFill;
		b.m_Color = color;
		return b;
	}

	FBrush FBrush::Image(const FName& imagePath, const FColor& tint)
	{
		FBrush b;
		b.m_BrushStyle = EBrushStyle::Image;
		b.m_ImagePath = imagePath;
		b.m_Color = tint;
		return b;
	}

	FBrush FBrush::Gradient(const FGradient& gradient, const FColor& tint)
	{
		FBrush b;
		b.m_BrushStyle = EBrushStyle::Gradient;
		b.m_Gradient = gradient;
		b.m_Color = tint;
		return b;
	}

	bool FBrush::IsValid()
	{
		switch (m_BrushStyle)
		{
		case EBrushStyle::SolidFill:
			return m_Color.a > 0;
		case EBrushStyle::Image:
			return m_ImagePath.IsValid();
		case EBrushStyle::Gradient:
			return m_Gradient.IsValid();
		case EBrushStyle::None:
			break;
		}

		return false;
	}

	bool FBrush::operator==(const FBrush& rhs) const
	{
		if (m_BrushStyle != rhs.m_BrushStyle)
			return false;
		if (m_Tiling != rhs.m_Tiling)
			return false;
		if (m_ImageFit != rhs.m_ImageFit)
			return false;
		if (m_BrushPos != rhs.m_BrushPos)
			return false;
		if (m_BrushSize != rhs.m_BrushSize)
			return false;

		switch (m_BrushStyle)
		{
		case EBrushStyle::SolidFill:
			return m_Color == rhs.m_Color;
		case EBrushStyle::Gradient:
			return m_Color == rhs.m_Color && m_Gradient == rhs.m_Gradient;
		case EBrushStyle::Image:
			return m_Color == rhs.m_Color && m_ImagePath == rhs.m_ImagePath && (m_ImageFit != EImageFit::NineSlice || m_SliceMargins == rhs.m_SliceMargins);
		case EBrushStyle::None:
			break;
		}

		return false;
	}

} // namespace Fusion
