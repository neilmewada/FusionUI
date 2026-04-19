#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FCompoundWidget::FCompoundWidget()
	{
		
	}

	void FCompoundWidget::SetParentSurfaceRecursive(Ref<FSurface> surface)
	{
		ZoneScoped;

		Super::SetParentSurfaceRecursive(surface);

		if (m_Child)
		{
			m_Child->SetParentSurfaceRecursive(surface);
		}
	}

	void FCompoundWidget::DetachChild(Ref<FWidget> child)
	{
		ZoneScoped;

		Super::DetachChild(child);

		if (m_Child == child && m_Child != nullptr)
		{
			if (Ref<FSurface> surface = GetParentSurface())
			{
				surface->MarkLayerTreeDirty();
			}

			m_Child->OnDetachedFromParent(this);

			m_Child->SetParentSurfaceRecursive(nullptr);
			m_Child->SetParentWidget(nullptr);
			m_Child = nullptr;

			MarkLayoutDirty();
			MarkPaintDirty();
		}
	}

	void FCompoundWidget::SetChild(Ref<FWidget> widget)
	{
		ZoneScoped;

		if (m_Child == widget)
			return;

		DetachChild(m_Child);

		if (widget)
		{
			widget->DetachFromParent();
		}

		m_Child = widget;

		if (m_Child)
		{
			if (m_Child->GetParentSurface() != GetParentSurface())
			{
				m_Child->SetParentSurfaceRecursive(GetParentSurface());
			}

			m_Child->SetParentWidget(this);
			m_Child->OnAttachedToParent(this);

			m_Child->UpdateBoundaryFlags();

			if (Ref<FSurface> surface = GetParentSurface())
			{
				surface->MarkLayerTreeDirty();
			}
		}

		MarkLayoutDirty();
		MarkPaintDirty();
	}

	FVec2 FCompoundWidget::MeasureContent(FVec2 availableSize)
	{
		FVec2 baseSize = GetMinimumContentSize();

		if (!m_Child || m_Child->IsExcluded())
		{
			return m_DesiredSize = ApplyLayoutConstraints(baseSize);
		}

		FMargin childMargin = m_Child->Margin();

		FVec2 childAvailableSize = FVec2(
			FMath::Max(0.0f, availableSize.x - (childMargin.left + childMargin.right + Padding().left + Padding().right)),
			FMath::Max(0.0f, availableSize.y - (childMargin.top + childMargin.bottom + Padding().top + Padding().bottom))
		);

		FVec2 childSize = m_Child->MeasureContent(childAvailableSize);

		return m_DesiredSize = ApplyLayoutConstraints(FVec2(
			childSize.x + childMargin.left + childMargin.right + Padding().left + Padding().right,
			childSize.y + childMargin.top + childMargin.bottom + Padding().top + Padding().bottom
		));
	}

	void FCompoundWidget::ArrangeContent(FVec2 finalSize)
	{
		Super::ArrangeContent(finalSize);
		// Always use layoutSize from base class, as it has the layout constraints applied.

		if (!m_Child || m_Child->IsExcluded())
			return;

		FMargin childMargin = m_Child->Margin();

		f32 childAreaWidth = FMath::Max(0.0f, GetLayoutSize().x - Padding().left - Padding().right - childMargin.left - childMargin.right);
		f32 childAreaHeight = FMath::Max(0.0f, GetLayoutSize().y - Padding().top - Padding().bottom - childMargin.top - childMargin.bottom);

		FVec2 childPos = FVec2(Padding().left + childMargin.left, Padding().top + childMargin.top);
		FVec2 childSize;

		switch (m_Child->HAlign())
		{
		case EHAlign::Auto:
		case EHAlign::Fill:
			childSize.x = childAreaWidth;
			break;
		case EHAlign::Left:
			childSize.x = FMath::Min(m_Child->GetDesiredSize().x, childAreaWidth);
			break;
		case EHAlign::Center:
			childSize.x = FMath::Min(m_Child->GetDesiredSize().x, childAreaWidth);
			childPos.x += (childAreaWidth - childSize.x) / 2.0f;
			break;
		case EHAlign::Right:
			childSize.x = FMath::Min(m_Child->GetDesiredSize().x, childAreaWidth);
			childPos.x += childAreaWidth - childSize.x;
			break;
		}

		switch (m_Child->VAlign())
		{
		case EVAlign::Auto:
		case EVAlign::Fill:
			childSize.y = childAreaHeight;
			break;
		case EVAlign::Top:
			childSize.y = FMath::Min(m_Child->GetDesiredSize().y, childAreaHeight);
			break;
		case EVAlign::Center:
			childSize.y = FMath::Min(m_Child->GetDesiredSize().y, childAreaHeight);
			childPos.y += (childAreaHeight - childSize.y) / 2.0f;
			break;
		case EVAlign::Bottom:
			childSize.y = FMath::Min(m_Child->GetDesiredSize().y, childAreaHeight);
			childPos.y += childAreaHeight - childSize.y;
			break;
		}

		m_Child->SetLayoutPosition(childPos);
		m_Child->ArrangeContent(childSize);
	}

	void FCompoundWidget::SetWidgetFlagInternal(EWidgetFlags flag, bool set)
	{
		SetWidgetFlag(flag, set);
	}
} // namespace Fusion
