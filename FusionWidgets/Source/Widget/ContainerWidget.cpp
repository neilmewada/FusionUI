#include "Fusion/Widgets.h"

namespace Fusion
{
	FContainerWidget::FContainerWidget()
	{
		
	}

	void FContainerWidget::SetParentSurfaceRecursive(Ref<FSurface> surface)
	{
		ZoneScoped;

		Super::SetParentSurfaceRecursive(surface);

		for (SizeT i = 0; i < m_Children.Size(); i++)
		{
			m_Children[i]->SetParentSurfaceRecursive(surface);
		}
	}

	void FContainerWidget::AddChildWidget(Ref<FWidget> childWidget)
	{
		ZoneScoped;

		if (!childWidget || m_Children.Contains(childWidget))
			return;

		childWidget->DetachFromParent();

		m_Children.Add(childWidget);

		childWidget->SetParentWidget(this);
		childWidget->SetParentSurfaceRecursive(GetParentSurface());
		childWidget->UpdateBoundaryFlags();

		childWidget->OnAttachedToParent(this);

		if (Ref<FSurface> surface = GetParentSurface())
		{
			surface->MarkLayerTreeDirty();
		}

		MarkLayoutDirty();
		MarkPaintDirty();
	}

	void FContainerWidget::RemoveChildWidget(Ref<FWidget> childWidget)
	{
		DetachChild(childWidget);
	}

	void FContainerWidget::DetachChild(Ref<FWidget> child)
	{
		ZoneScoped;

		Super::DetachChild(child);

		if (const int index = (int)m_Children.IndexOf(child); index >= 0)
		{
			if (Ref<FSurface> surface = GetParentSurface())
			{
				surface->MarkLayerTreeDirty();
			}

			child->OnDetachedFromParent(this);

			m_Children.RemoveAt(index);
			child->SetParentWidget(nullptr);
			child->SetParentSurfaceRecursive(nullptr);

			MarkLayoutDirty();
			MarkPaintDirty();
		}
	}

	void FContainerWidget::SetWidgetFlagInternal(EWidgetFlags flag, bool set)
	{
		SetWidgetFlag(flag, set);
	}
} // namespace Fusion
