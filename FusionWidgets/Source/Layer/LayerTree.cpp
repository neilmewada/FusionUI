#include "Fusion/Widgets.h"

namespace Fusion
{

	void FLayerTree::MarkSyncNeeded()
	{
		needsSync = true;
	}

	void FLayerTree::DoSyncIfNeeded(FWidget* rootWidget)
	{
		if (!needsSync)
			return;

		ZoneScoped;

		HashSet<Uuid> visited;
		SyncWidget(rootWidget, nullptr, visited);

		HashSet<Uuid> widgetsToRemove;

		for (const auto& [widgetUuid, layer] : widgetUuidToLayerMap)
		{
			if (!visited.Exists(widgetUuid))
			{
				widgetsToRemove.Add(widgetUuid);
			}
		}

		for (auto widgetUuid : widgetsToRemove)
		{
			widgetUuidToLayerMap.Remove(widgetUuid);
		}

		needsSync = false;
	}

	void FLayerTree::DoPaintIfNeeded()
	{
		if (!rootLayer)
			return;

		rootLayer->cachedTransformInParentLayerSpace = FAffineTransform::Identity();
		rootLayer->DoPaintIfNeeded();
	}

	Ptr<FLayer> FLayerTree::FindLayerForWidget(FUuid widgetUuid)
	{
		auto it = widgetUuidToLayerMap.Find(widgetUuid);
		if (it != widgetUuidToLayerMap.End())
		{
			return it->second;
		}
		return nullptr;
	}

	void FLayerTree::SyncWidget(FWidget* widget, Ptr<FLayer> parentLayer, HashSet<FUuid>& visited)
	{
		if (!widget)
			return;

		ZoneScoped;

		Ptr<FLayer> currentLayer = parentLayer;

		if (widget->IsPaintBoundary())
		{
			const FUuid uuid = widget->GetUuid();

			Ptr<FLayer> layer;

			auto it = widgetUuidToLayerMap.Find(uuid);

			if (it != widgetUuidToLayerMap.End())
			{
				layer = it->second;
			}
			else
			{
				layer = new FLayer("Layer", this);

				layer->owningWidget = widget;
				layer->ownerTree = this;
				widgetUuidToLayerMap.Add(uuid, layer);
			}

			layer->parent = parentLayer;
			layer->children.Clear();

			if (parentLayer != nullptr)
				parentLayer->children.Add(layer);
			else
				rootLayer = layer;

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
