#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

	FWidget::FWidget() : Super("Widget")
	{
		m_MaxHeight = TNumericLimits<f32>::Infinity();
		m_MaxWidth = TNumericLimits<f32>::Infinity();
		m_Pivot = FVec2(0.5f, 0.5f);
		m_Opacity = 1.0f;
	}

	FWidget::~FWidget()
	{

	}

	void FWidget::MarkPaintDirty()
	{
		ZoneScoped;

		if (IsPaintDirty())
			return;

		m_WidgetFlags |= EWidgetFlags::PaintDirty;

		if (Ref<FSurface> surface = m_ParentSurface.Lock())
		{
			Ref<FWidget> parent = this;

			while (parent != nullptr)
			{
				if (parent != this && parent->IsPaintDirty())
				{
					break;
				}

				if (parent->IsPaintBoundary())
				{
					parent->SetWidgetFlag(EWidgetFlags::PaintDirty, true);
					break;
				}

				parent = parent->GetParentWidget();
			}
		}
	}

	void FWidget::MarkLayoutDirty()
	{
		ZoneScoped;

		if (IsLayoutDirty())
			return;

		m_WidgetFlags |= EWidgetFlags::LayoutDirty;

		if (Ref<FSurface> surface = m_ParentSurface.Lock())
		{
			Ref<FWidget> parent = this;

			while (parent != nullptr)
			{
				if (parent != this && parent->IsLayoutDirty())
				{
					break;
				}

				if (parent->IsLayoutBoundary())
				{
					surface->AddPendingLayoutRoot(parent);
					break;
				}

				parent = parent->GetParentWidget();
			}
		}
	}

	void FWidget::OnConstruct()
	{
		Super::OnConstruct();

		m_WidgetFlags |= EWidgetFlags::PendingConstruction;

		FUSION_TRY
		{
			Construct();
		}
		FUSION_CATCH (const FException& exception)
		{
			FUSION_LOG_ERROR("Widget", "Exception occurred during widget [{}] construction: {}\n{}", GetClassName(), exception.what(), exception.GetStackTraceString(true));
			m_WidgetFlags |= EWidgetFlags::Faulted;
		}

		m_WidgetFlags &= ~EWidgetFlags::PendingConstruction;
	}

	void FWidget::OnAttachedToParent(Ref<FWidget> parent)
	{
		RefreshStyle();
	}

	void FWidget::OnDetachedFromParent(Ref<FWidget> parent)
	{
		RefreshStyle();
	}

	Ref<FWidget> FWidget::FindSubWidgetTypeInHierarchy(FTypeID widgetClassId)
	{
		if (IsOfType(widgetClassId))
			return this;

		ZoneScoped;

		int childCount = GetChildCount();
		for (int i = 0; i < childCount; i++)
		{
			if (Ref<FWidget> child = GetChildAt(i))
			{
				if (child->IsOfType(widgetClassId))
				{
					return child;
				}
				if (auto result = child->FindSubWidgetTypeInHierarchy(widgetClassId))
				{
					return result;
				}
			}
		}
		return nullptr;
	}

	Ref<FApplicationInstance> FWidget::GetApplication() const
	{
		if (Ref<FSurface> surface = GetParentSurface())
		{
			return surface->GetApplication();
		}
		return nullptr;
	}

	void FWidget::OnPropertyModified(const FName& propertyName)
	{
		thread_local const FName styleProperty = "Style";
		thread_local const FName subStyleProperty = "SubStyle";
		thread_local const FName opacityProperty = "Opacity";
		thread_local const FName pivotProperty = "Pivot";

		if (propertyName == styleProperty || propertyName == subStyleProperty)
		{
			m_CachedStyle = nullptr;
			m_StyleCached = false;

			for (int i = 0; i < GetChildCount(); i++)
			{
				if (Ref<FWidget> child = GetChildAt(i))
				{
					if (child->m_SubStyle.IsValid()) // Child's resolved style name is dependent on this widget
					{
						child->RefreshStyle();
					}
				}
			}
		}
		else if (propertyName == opacityProperty)
		{
			UpdateBoundaryFlags();
		}
		else if (propertyName == pivotProperty)
		{
			m_Pivot.x = FMath::Clamp01(m_Pivot.x);
			m_Pivot.y = FMath::Clamp01(m_Pivot.y);
		}
	}

	FAffineTransform FWidget::GetGlobalTransform() const
	{
		ZoneScoped;

		// Walk up to the nearest paint boundary (which owns a layer)
		const FWidget* boundary = this;
		while (boundary != nullptr && !boundary->IsPaintBoundary())
		{
			boundary = boundary->GetParentWidget().Get();
		}

		if (boundary == nullptr)
			return m_CachedLayerSpaceTransform;

		if (Ref<FSurface> surface = m_ParentSurface.Lock())
		{
			if (Ref<FLayer> layer = surface->GetLayerTree()->FindLayerForWidget(boundary->GetUuid()))
			{
				return layer->GetGlobalTransform() * m_CachedLayerSpaceTransform;
			}
		}
		
		return m_CachedLayerSpaceTransform;
	}

	FAffineTransform FWidget::GetChildTransform()
	{
		return FAffineTransform::Translation(GetLayoutPosition()) *
			FAffineTransform::Translation(m_Pivot) *
			Transform() *
			FAffineTransform::Translation(-m_Pivot);
	}

	int FWidget::GetIndexOfChild(Ref<FWidget> child)
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			if (GetChildAt(i) == child)
				return i;
		}

		return -1;
	}

	EStyleState FWidget::GetStyleState()
	{
		return m_StyleState;
	}

	bool FWidget::IsLayoutBoundary()
	{
		ZoneScoped;

		const bool isFixedSize = FMath::ApproxEquals(m_MinWidth, m_MaxWidth) && FMath::ApproxEquals(m_MinHeight, m_MaxHeight);
		if (isFixedSize)
			return true;

		const bool isRootWidget = m_ParentWidget.IsNull() && m_ParentSurface.IsValid();
		if (isRootWidget)
			return true;

		return false;
	}

	void FWidget::SetLayoutPosition(FVec2 newPosition)
	{
		if (m_LayoutPosition == newPosition)
			return;

		m_LayoutPosition = newPosition;

		MarkPaintDirty();
	}

	FVec2 FWidget::GetMinimumContentSize()
	{
		return FVec2(m_MinWidth + m_Padding.left + m_Padding.right, m_MinHeight + m_Padding.top + m_Padding.bottom);
	}

	FVec2 FWidget::ApplyLayoutConstraints(FVec2 desiredSize)
	{
		ZoneScoped;

		f32 constrainedWidth = FMath::Clamp(desiredSize.x, m_MinWidth + m_Padding.left + m_Padding.right, m_MaxWidth + m_Padding.left + m_Padding.right);
		f32 constrainedHeight = FMath::Clamp(desiredSize.y, m_MinHeight + m_Padding.top + m_Padding.bottom, m_MaxHeight + m_Padding.top + m_Padding.bottom);

		return FVec2(constrainedWidth, constrainedHeight);
	}

	FVec2 FWidget::MeasureContent([[maybe_unused]] FVec2 availableSize)
	{
		return m_DesiredSize = GetMinimumContentSize();
	}

	void FWidget::ArrangeContent(FVec2 finalSize)
	{
		ArrangeContentBase(finalSize);
	}

	void FWidget::ArrangeContentBase(FVec2 finalSize)
	{
		ZoneScoped;

		m_WidgetFlags &= ~EWidgetFlags::LayoutDirty;

		m_LayoutSize = ApplyLayoutConstraints(finalSize);

		MarkPaintDirty();
	}

	void FWidget::RefreshStyle()
	{
		m_CachedStyle = ResolveStyle();
		m_StyleCached = true;

		if (m_CachedStyle)
			ApplyStyle(*m_CachedStyle);
	}

	void FWidget::ApplyStyle()
	{
		if (!m_StyleCached)
		{
			RefreshStyle();
		}
		else if (m_CachedStyle)
		{
			ApplyStyle(*m_CachedStyle);
		}
	}

	void FWidget::RefreshStyleRecursively()
	{
		RefreshStyle();

		for (int i = 0; i < GetChildCount(); i++)
		{
			if (Ref<FWidget> widget = GetChildAt(i))
			{
				widget->RefreshStyleRecursively();
			}
		}
	}

	FName FWidget::ResolveStyleName()
	{
		ZoneScoped;

		if (m_Style.IsValid())
			return m_Style;

		if (m_SubStyle.IsValid())
		{
			Ref<FWidget> parent = GetParentWidget();
			while (parent != nullptr)
			{
				if (parent->IsStyleScopeBoundary() || parent->m_SubStyle.IsValid())
				{
					return parent->ResolveStyleName().ToString() + "/" + m_SubStyle.ToString();
				}

				parent = parent->GetParentWidget();
			}
		}
		return GetClassName();
	}

	Ref<FStyle> FWidget::ResolveStyle()
	{
		ZoneScoped;

		FName styleName = ResolveStyleName();

		if (Ref<FSurface> surface = GetParentSurface())
		{
			if (Ref<FTheme> theme = surface->GetTheme())
			{
				return theme->FindStyle(styleName);
			}
		}

		return nullptr;
	}

	void FWidget::SetParentSurfaceRecursive(Ref<FSurface> surface)
	{
		ZoneScoped;

		this->m_ParentSurface = surface;

		const bool wasPaintDirty = IsPaintDirty();
		const bool wasLayoutDirty = IsLayoutDirty();
		m_WidgetFlags &= ~(EWidgetFlags::PaintDirty | EWidgetFlags::LayoutDirty);

		if (wasPaintDirty)
			MarkPaintDirty();
		if (wasLayoutDirty)
			MarkLayoutDirty();
	}

	void FWidget::DetachFromParent()
	{
		if (Ref<FWidget> parent = m_ParentWidget.Lock())
		{
			parent->DetachChildWidget(this);
		}
	}

	bool FWidget::IsPaintBoundary() const
	{
		return TestWidgetFlags(EWidgetFlags::CachedPaintBoundary) || TestWidgetFlags(EWidgetFlags::CachedCompositingBoundary);
	}

	bool FWidget::IsCompositingBoundary() const
	{
		return TestWidgetFlags(EWidgetFlags::CachedCompositingBoundary);
	}

	void FWidget::UpdateBoundaryFlags()
	{
		bool isCompositing = (m_Opacity < 0.999f) || FEnumHasFlag(m_WidgetFlags, EWidgetFlags::ForceCompositingBoundary);
		bool isPaint = (m_ParentWidget.IsNull()) || FEnumHasFlag(m_WidgetFlags, EWidgetFlags::ForcePaintBoundary) || isCompositing;

		bool wasCompositing = FEnumHasFlag(m_WidgetFlags, EWidgetFlags::CachedCompositingBoundary);
		bool wasPaint = FEnumHasFlag(m_WidgetFlags, EWidgetFlags::CachedPaintBoundary);

		SetWidgetFlag(EWidgetFlags::CachedCompositingBoundary, isCompositing);
		SetWidgetFlag(EWidgetFlags::CachedPaintBoundary, isPaint);

		if (isCompositing != wasCompositing || isPaint != wasPaint)
		{
			if (Ref<FSurface> surface = m_ParentSurface.Lock())
				surface->MarkLayerTreeDirty();
		}
	}

	void FWidget::Paint([[maybe_unused]] FPainter& painter)
	{
	}

	void FWidget::PaintOverContent([[maybe_unused]] FPainter& painter)
	{
	}

	bool FWidget::SelfHitTest(FVec2 localMousePos)
	{
		ZoneScoped;

		if (Excluded() || IsFaulted() || !Visible())
			return false;

		return FRect::FromSize(FVec2(), m_LayoutSize).Contains(localMousePos);
	}

	void FWidget::SetFaulted()
	{
		SetWidgetFlag(EWidgetFlags::Faulted, true);

		if (Ref<FWidget> parent = GetParentWidget())
		{
			parent->MarkLayoutDirty();
			parent->MarkPaintDirty();

			parent->DetachChild(this);
		}

		m_ParentWidget = nullptr;
	}

	void FWidget::AttachChildWidget(Ref<FWidget> child)
	{
		child->DetachFromParent();
		child->SetParentWidget(this);
		child->SetParentSurfaceRecursive(GetParentSurface());
		child->UpdateBoundaryFlags();
		child->OnAttachedToParent(this);

		if (Ref<FSurface> surface = GetParentSurface())
			surface->MarkLayerTreeDirty();

		MarkLayoutDirty();
		MarkPaintDirty();
	}

	void FWidget::DetachChildWidget(Ref<FWidget> child)
	{
		child->OnDetachedFromParent(this);
		child->SetParentSurfaceRecursive(nullptr);
		child->SetParentWidget(nullptr);

		if (Ref<FSurface> surface = GetParentSurface())
			surface->MarkLayerTreeDirty();

		MarkLayoutDirty();
		MarkPaintDirty();
	}

	void FWidget::SetWidgetFlag(EWidgetFlags flag, bool set)
	{
		if (set)
		{
			m_WidgetFlags |= flag;
		}
		else
		{
			m_WidgetFlags &= ~flag;
		}
	}

	void FWidget::SetStyleStateFlag(EStyleState state, bool set)
	{
		EStyleState prev = m_StyleState;

		if (set)
			m_StyleState |= state;
		else
			m_StyleState &= ~state;

		if (FEnumHasFlag(state, EStyleState::Disabled))
		{
			if (set)
				OnDisabled();
			else
				OnEnabled();
		}

		if (m_StyleState != prev)
		{
			if (m_StyleCached)
			{
				if (m_CachedStyle)
					ApplyStyle(*m_CachedStyle);
			}
			else
			{
				RefreshStyle();
			}

			if ((PropagatedStyleStates() & state) != 0)
			{
				for (int i = 0; i < GetChildCount(); i++)
				{
					if (Ref<FWidget> child = GetChildAt(i))
					{
						child->SetStyleStateFlag(PropagatedStyleStates() & state, set);
					}
				}
			}
		}
	}

	void FWidget::SetEnabledRecursive(bool enabled, Ref<FWidget> parent)
	{
		if (TestStyleState(EStyleState::Disabled) == !enabled)
			return;

		ZoneScoped;

		if (parent && !parent->Enabled())
		{
			enabled = false;
		}

		SetStyleStateFlag(EStyleState::Disabled, !enabled);
		
		for (int i = 0; i < GetChildCount(); i++)
		{
			if (Ref<FWidget> widget = GetChildAt(i))
				widget->SetEnabledRecursive(enabled, this);
		}
	}

	bool FWidget::IsAncestorDisabled()
	{
		if (TestStyleState(EStyleState::Disabled))
			return true;
		Ref<FWidget> parent = GetParentWidget();
		return parent.IsValid() && parent->IsAncestorDisabled();
	}

	bool FWidget::IsAncestorExcluded()
	{
		if (IsExcluded())
			return true;
		Ref<FWidget> parent = GetParentWidget();
		return parent.IsValid() && parent->IsAncestorExcluded();
	}

	bool FWidget::IsAncestorHidden()
	{
		if (IsHidden())
			return true;
		Ref<FWidget> parent = GetParentWidget();
		return parent.IsValid() && parent->IsAncestorHidden();
	}
} // namespace Fusion
