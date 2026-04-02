#include "Fusion/Widgets.h"

namespace Fusion
{

	void FLayerTree::MarkSyncNeeded()
	{
		m_NeedsSync = true;
	}

	void FLayerTree::DoSyncIfNeeded(FWidget* rootWidget)
	{
		if (!m_NeedsSync)
			return;

		ZoneScoped;

		FHashSet<FUuid> visited;
		SyncWidget(rootWidget, nullptr, visited);

		FHashSet<FUuid> widgetsToRemove;

		for (const auto& [widgetUuid, layer] : m_WidgetUuidToLayerMap)
		{
			if (!visited.Contains(widgetUuid))
			{
				widgetsToRemove.Add(widgetUuid);
			}
		}

		for (auto widgetUuid : widgetsToRemove)
		{
			m_WidgetUuidToLayerMap.Remove(widgetUuid);
		}

		m_NeedsSync = false;
	}

	void FLayerTree::DoPaintIfNeeded()
	{
		if (!m_RootLayer)
			return;

		m_RootLayer->cachedTransformInParentLayerSpace = FAffineTransform::Identity();
		m_RootLayer->DoPaintIfNeeded();
	}

	Ptr<FLayer> FLayerTree::FindLayerForWidget(FUuid widgetUuid)
	{
		auto it = m_WidgetUuidToLayerMap.Find(widgetUuid);
		if (it != m_WidgetUuidToLayerMap.End())
		{
			return it->second;
		}
		return nullptr;
	}

	void FLayerTree::SyncWidget(Ptr<FWidget> widget, Ptr<FLayer> parentLayer, FHashSet<FUuid>& visited)
	{
		if (!widget)
			return;

		ZoneScoped;

		Ptr<FLayer> currentLayer = parentLayer;

		if (widget->IsPaintBoundary())
		{
			const FUuid uuid = widget->GetUuid();

			Ptr<FLayer> layer;

			auto it = m_WidgetUuidToLayerMap.Find(uuid);

			if (it != m_WidgetUuidToLayerMap.End())
			{
				layer = it->second;
			}
			else
			{
				layer = new FLayer("Layer", this);

				layer->m_OwningWidget = widget;
				layer->m_OwnerTree = Ptr(this);
				m_WidgetUuidToLayerMap.Add(uuid, layer);
			}

			layer->parent = parentLayer;
			layer->children.Clear();

			if (parentLayer != nullptr)
				parentLayer->children.Add(layer);
			else
				m_RootLayer = layer;

			visited.Add(uuid);
			currentLayer = layer.Get();
		}

		for (u32 i = 0; i < widget->GetChildCount(); i++)
		{
			if (Ptr<FWidget> child = widget->GetChildAt(i))
			{
				SyncWidget(child.Get(), currentLayer, visited);
			}
		}
	}
    
} // namespace Fusion
