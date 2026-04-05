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

		painter.SetBrush(m_Background);
		painter.SetPen(m_Border);

		FVec2 layoutSize = GetLayoutSize();

		painter.FillAndStrokeShape(FRect(0, 0, layoutSize.width, layoutSize.height), m_Shape);
	}

	void FDecoratedWidget::ApplyStyle(FStyle& style)
	{
		Super::ApplyStyle(style);


	}

} // namespace Fusion
