#include "Fusion/Widgets.h"

namespace Fusion
{
	FSurface::FSurface()
	{
		m_LayerTree = NewObject<FLayerTree>("LayerTree");
	}

	void FSurface::AddPendingLayoutRoot(Ptr<FWidget> layoutRoot)
	{
		if (!layoutRoot)
			return;

		if (m_PendingLayoutRootIds.Contains(layoutRoot->GetUuid()))
			return;

		m_PendingLayoutRoots.Add(layoutRoot);
		m_PendingLayoutRootIds.Add(layoutRoot->GetUuid());
	}

	void FSurface::MarkRootLayoutDirty()
	{
		if (!m_RootWidget)
			return;

		AddPendingLayoutRoot(m_RootWidget);

		m_RootWidget->MarkLayoutDirty();
		m_RootWidget->MarkPaintDirty();
	}

	void FSurface::MarkLayerTreeDirty()
	{
		m_LayerTree->MarkSyncNeeded();
	}
} // namespace Fusion
