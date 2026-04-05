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

		painter.SetBrush(Background());
		painter.SetPen(Border());

		FVec2 layoutSize = GetLayoutSize();

		painter.FillAndStrokeShape(FRect(0, 0, layoutSize.width, layoutSize.height), Shape());
	}

	void FDecoratedWidget::ApplyStyle(FStyle& style)
	{
		Super::ApplyStyle(style);

		FUSION_APPLY_STYLES(Background, Border, Shape);
	}

} // namespace Fusion
