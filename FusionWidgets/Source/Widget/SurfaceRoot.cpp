#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FSurfaceRoot::FSurfaceRoot()
	{
		
	}

	void FSurfaceRoot::Construct()
	{
		Super::Construct();

		OverlayStack(
			FNew(FContainerWidget)
			.SelfHitTestEnabled(false)
		);
	}

	FVec2 FSurfaceRoot::MeasureContent(FVec2 availableSize)
	{
		if (m_Content && !m_Content->IsExcluded())
		{
			m_Content->MeasureContent(availableSize);
		}

		if (m_OverlayStack && !m_OverlayStack->IsExcluded())
		{
			m_OverlayStack->MeasureContent(availableSize);
		}

		return m_DesiredSize = ApplyLayoutConstraints(availableSize);
	}

	void FSurfaceRoot::ArrangeContent(FVec2 finalSize)
	{
		ArrangeContentBase(finalSize);

		const FVec2 layoutSize = GetLayoutSize();

		if (m_Content && !m_Content->IsExcluded())
		{
			m_Content->SetLayoutPosition(FVec2(0.0f));
			m_Content->ArrangeContent(layoutSize);
		}

		if (m_OverlayStack && !m_OverlayStack->IsExcluded())
		{
			m_OverlayStack->SetLayoutPosition(FVec2(0.0f));
			m_OverlayStack->ArrangeContent(layoutSize);
		}
	}

}
