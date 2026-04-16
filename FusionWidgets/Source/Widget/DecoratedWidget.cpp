#include "Fusion/Widgets.h"

namespace Fusion
{
	FDecoratedWidget::FDecoratedWidget()
	{
		m_Shape = FRectangle();
	}

	void FDecoratedWidget::Paint(FPainter& painter)
	{
		Super::Paint(painter);

		FVec2 layoutSize = GetLayoutSize();
		FRect widgetRect(0, 0, layoutSize.width, layoutSize.height);

		painter.SetBrush(Background());
		painter.SetPen(Border());
		painter.FillAndStrokeShape(widgetRect, Shape());
	}

	void FDecoratedWidget::PaintOverContent(FPainter& painter)
	{
		Super::PaintOverContent(painter);

		FVec2 layoutSize = GetLayoutSize();
		FRect widgetRect(0, 0, layoutSize.width, layoutSize.height);

		FPen outline = Outline();
		if (outline.IsValid())
		{
			painter.SetClipEnabled(false);

			// Expand outward by half the stroke thickness + 2px gap so the
			// outline sits cleanly outside the border without overlapping it.
			FRect outlineRect = widgetRect.Expand(outline.GetThickness() * 0.5f + 2.0f);
			painter.SetBrush(FBrush());
			painter.SetPen(outline);
			painter.StrokeShape(outlineRect, Shape());

			painter.SetClipEnabled(true);
		}
	}
} // namespace Fusion
