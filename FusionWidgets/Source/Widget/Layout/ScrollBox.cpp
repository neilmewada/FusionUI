#include "Fusion/Widgets.h"

namespace Fusion
{
	FScrollBox::FScrollBox() : Super()
	{
		m_CanScrollVertical = true;
		m_CanScrollHorizontal = false;
		m_ScrollOffset = FVec2(0, 0);
	}

	FVec2 FScrollBox::MeasureContent(FVec2 availableSize)
	{
		return Super::MeasureContent(availableSize);
	}

	void FScrollBox::ArrangeContent(FVec2 finalSize)
	{
		Super::ArrangeContent(finalSize);
	}

} // namespace Fusion
