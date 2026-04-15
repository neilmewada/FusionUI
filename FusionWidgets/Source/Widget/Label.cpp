#include "Fusion/Widgets.h"

namespace Fusion
{
	FLabel::FLabel()
	{
		m_Color = FColors::White;
		m_Font = FFont::Regular(FFont::kDefaultFamilyName, 16);
	}

	FVec2 FLabel::MeasureContent(FVec2 availableSize)
	{
		if (Text().Empty() || !Font().Valid())
			return m_DesiredSize = FVec2();

		FFontAtlas* atlas = GetApplication()->GetFontAtlas().Get();
		const f32 scale = Font().GetPointSize() / (f32)FFontAtlas::kSdfRenderSize;
		FFontMetrics metrics = atlas->GetScaledMetrics(Font());

		f32 width = 0.0f;
		for (char32_t cp : Text().Codepoints())
		{
			FGlyph glyph = atlas->FindOrAddGlyph(Font(), cp);
			if (glyph.IsValid())
				width += (f32)glyph.Advance * scale;
		}

		return m_DesiredSize = FVec2(width, metrics.Ascender - metrics.Descender);
	}

	void FLabel::ArrangeContent(FVec2 finalSize)
	{
		Super::ArrangeContent(finalSize);
	}

	void FLabel::Paint(FPainter& painter)
	{
		Super::Paint(painter);

		if (Text().Empty() || !Font().Valid())
			return;

		FFontAtlas* atlas = GetApplication()->GetFontAtlas().Get();
		FFontMetrics metrics = atlas->GetScaledMetrics(Font());

		painter.SetFont(Font());
		painter.SetPen(FPen(Color()));
		painter.DrawText(FVec2(0, metrics.Ascender), Text());
	}

} // namespace Fusion
