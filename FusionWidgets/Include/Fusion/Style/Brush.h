#pragma once

namespace Fusion
{
	// Determines what content the brush renders.

	enum class EBrushStyle : u8
	{
		None = 0,
		SolidFill,  // Filled with a solid color (uses color)
		Image,      // Textured fill (uses imageName, color as tint, imageFit, brushSize, brushPos)
		Gradient    // Gradient fill (uses gradient, color as tint)
	};
	FUSION_ENUM_CLASS(EBrushStyle);

	// Controls how an image brush tiles when brushSize is smaller than the widget rect.
	// Only applies to FBrushStyle::Image. Mutually exclusive with NineSlice.
	enum class EBrushTiling : u8
	{
		None = 0,
		TileX,   // Repeat horizontally
		TileY,   // Repeat vertically
		TileXY   // Repeat in both axes
	};
	FUSION_ENUM_CLASS(EBrushTiling);

	// Controls how an image brush is fitted into the widget rect.
	// FPainter computes vertex UVs from this at tessellation time.
	enum class EImageFit : u8
	{
		Fill = 0,	// Stretch to fill the widget rect exactly (Default. ignores aspect ratio)
		Auto,		// Renders the image at its natural size.
		Contain,	// Scale uniformly to fit within the rect, preserving aspect ratio (may letterbox)
		Cover,		// Scale uniformly to fill the rect, preserving aspect ratio (may crop)
		NineSlice	// Fixed corners, stretched edges/center. Requires sliceMargins to be set.
	};
	FUSION_ENUM_CLASS(EImageFit);

    class FUSIONWIDGETS_API FBrush final
    {
    public:

		FBrush() = default;

		FBrush(const FColor& color);
		FBrush(const FName& imagePath, const FColor& tint = FColors::White);
		FBrush(const FGradient& gradient, const FColor& tint = FColors::White);

		// Named factories
		static FBrush Solid(const FColor& color);
		static FBrush Image(const FName& imagePath, const FColor& tint = FColors::White);
		static FBrush Gradient(const FGradient& gradient, const FColor& tint = FColors::White);

		bool IsValid();

		bool IsTiled() const { return m_Tiling != EBrushTiling::None; }

		EBrushStyle  GetBrushStyle()  const { return m_BrushStyle; }
		EBrushTiling GetBrushTiling() const { return m_Tiling; }
		const FGradient& GetGradient() const { return m_Gradient; }
		FGradient& GetGradient()			 { return m_Gradient; }

		// For SolidFill: the fill color. For Image/Gradient: the tint color (multiplied with texture/gradient output).
		const FColor& GetColor()     const { return m_Color; }
		const FName&  GetImagePath() const { return m_ImagePath; }
		EImageFit     GetImageFit()  const { return m_ImageFit; }

		// Border sizes for NineSlice fitting (left, top, right, bottom in pixels).
		// Only used when imageFit == FImageFit::NineSlice.
		const FMargin& GetSliceMargins() const { return m_SliceMargins; }

		// Explicit pixel size of the image within the widget rect. Vec2(-1,-1) = auto (driven by imageFit).
		const FVec2& GetBrushSize()     const { return m_BrushSize; }

		// Normalized anchor point for image placement within the widget rect.
		// (0,0) = top-left, (0.5,0.5) = centered, (1,1) = bottom-right. Equivalent to CSS background-position.
		const FVec2& GetBrushPosition() const { return m_BrushPos; }

		// Fluent setters
		FBrush& Color(const FColor& color)           { m_Color = color;          return *this; }
		FBrush& ImageFit(EImageFit fit)                   { m_ImageFit = fit;         return *this; }
		FBrush& BrushTiling(EBrushTiling tiling)          { m_Tiling = tiling;        return *this; }
		FBrush& SliceMargins(const FMargin& margins) { m_SliceMargins = margins; return *this; }
		FBrush& BrushSize(FVec2 size)                { m_BrushSize = size;       return *this; }
		FBrush& BrushPosition(FVec2 pos)             { m_BrushPos = pos;         return *this; }

		bool operator==(const FBrush& rhs) const;

		bool operator!=(const FBrush& rhs) const
		{
			return !operator==(rhs);
		}

    private:

		FGradient m_Gradient;
		FColor    m_Color;
		FName     m_ImagePath;

		FVec2 m_BrushSize = FVec2(-1, -1);
		FVec2 m_BrushPos  = FVec2(0.5f, 0.5f);

		FMargin m_SliceMargins;

		EBrushTiling m_Tiling    = EBrushTiling::None;
		EBrushStyle  m_BrushStyle = EBrushStyle::None;
		EImageFit    m_ImageFit   = EImageFit::Fill;

    };

} // namespace Fusion
