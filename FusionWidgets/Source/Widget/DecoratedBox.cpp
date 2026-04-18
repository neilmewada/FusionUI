#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FDecoratedBox::FDecoratedBox()
	{
		m_Shape         = FRectangle();
		m_OutlineOffset = 2.0f;
	}

	void FDecoratedBox::Paint(FPainter& painter)
	{
		Super::Paint(painter);

		FVec2 layoutSize = GetLayoutSize();
		FRect widgetRect(0, 0, layoutSize.width, layoutSize.height);

		painter.SetBrush(Background());
		painter.SetPen(Border());
		painter.FillAndStrokeShape(widgetRect, Shape());
	}

	void FDecoratedBox::PaintOverContent(FPainter& painter)
	{
		Super::PaintOverContent(painter);

		if (!Enabled())
			return;

		FVec2 layoutSize = GetLayoutSize();
		FRect widgetRect(0, 0, layoutSize.width, layoutSize.height);

		FPen outline = Outline();
		if (Outline().IsValid())
		{
			painter.SetClipEnabled(false);

			FRect outlineRect = widgetRect.Expand(outline.GetThickness() * 0.5f + OutlineOffset());
			painter.SetBrush(FBrush());
			painter.SetPen(outline);
			painter.StrokeShape(outlineRect, Shape());

			painter.SetClipEnabled(true);
		}
	}
} // namespace Fusion
