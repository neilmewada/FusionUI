#include "Fusion/Widgets.h"

namespace Fusion
{

	FWidget::FWidget(FName name, Ptr<FObject> outer) : Super(name, outer)
	{
		m_MaxHeight = FNumericLimits<f32>::Infinity();
		m_MaxWidth = FNumericLimits<f32>::Infinity();
		m_Pivot = FVec2(0.5f, 0.5f);
		m_Opacity = 1.0f;
	}

	void FWidget::MarkPaintDirty()
	{
		ZoneScoped;

		if (IsPaintDirty())
			return;

		m_WidgetFlags |= EWidgetFlags::PaintDirty;


	}

	void FWidget::MarkLayoutDirty()
	{
	}

	void FWidget::OnConstruct()
	{
		Super::OnConstruct();

		FUSION_TRY
		{
			Construct();
		}
		FUSION_CATCH (const FException& exception)
		{
			FUSION_LOG_ERROR("Widget", "Exception occurred during widget [{}] construction: {}\n{}", GetClassName(), exception.what(), exception.GetStackTraceString(true));
			m_WidgetFlags |= EWidgetFlags::Faulted;
		}
	}

	void FWidget::OnPropertyModified(const FName& propertyName)
	{

	}

	FAffineTransform FWidget::GetGlobalTransform() const
	{
		// Walk up to the nearest paint boundary (which owns a layer)
		const FWidget* boundary = this;
		while (boundary != nullptr && !boundary->IsPaintBoundary())
		{
			boundary = boundary->GetParentWidget().Get();
		}

		if (boundary == nullptr)
			return m_CachedLayerSpaceTransform;

		if (Ptr<FSurface> surface = m_ParentSurface.Lock())
		{
			if (Ptr<FLayer> layer = surface->GetLayerTree()->FindLayerForWidget(boundary->GetUuid()))
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
			m_Transform *
			FAffineTransform::Translation(-m_Pivot);
	}

	void FWidget::SetLayoutPosition(FVec2 newPosition)
	{
		if (m_LayoutPosition == newPosition)
			return;

		m_LayoutPosition = newPosition;

		MarkPaintDirty();
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
			if (Ptr<FSurface> surface = m_ParentSurface.Lock())
				surface->MarkLayerTreeDirty();
		}
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
} // namespace Fusion
