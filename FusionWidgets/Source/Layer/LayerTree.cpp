#include "Fusion/Widgets.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
	FLayerTree::FLayerTree() : Super("LayerTree")
	{
		m_OverlayLayer = CreateSubobject<FLayer>("OverlayLayer");
	}

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

		m_RootLayer->m_CachedTransformInParentLayerSpace = FAffineTransform::Identity();
		m_RootLayer->DoPaintIfNeeded();
	}

	Ref<FLayer> FLayerTree::FindLayerForWidget(FUuid widgetUuid)
	{
		auto it = m_WidgetUuidToLayerMap.Find(widgetUuid);
		if (it != m_WidgetUuidToLayerMap.End())
		{
			return it->second;
		}
		return nullptr;
	}

	void FLayerTree::SyncWidget(Ref<FWidget> widget, Ref<FLayer> parentLayer, FHashSet<FUuid>& visited)
	{
		if (!widget)
			return;

		ZoneScoped;

		Ref<FLayer> currentLayer = parentLayer;

		if (widget->IsPaintBoundary())
		{
			const FUuid uuid = widget->GetUuid();

			Ref<FLayer> layer;

			auto it = m_WidgetUuidToLayerMap.Find(uuid);

			if (it != m_WidgetUuidToLayerMap.End())
			{
				layer = it->second;
			}
			else
			{
				layer = NewObject<FLayer>(this, "Layer");

				layer->m_OwningWidget = widget;
				layer->m_OwnerTree = Ref(this);
				m_WidgetUuidToLayerMap.Add(uuid, layer);
			}

			layer->m_Parent = parentLayer;
			layer->m_Children.Clear();

			if (parentLayer != nullptr)
				parentLayer->m_Children.Add(layer);
			else
				m_RootLayer = layer;

			visited.Add(uuid);
			currentLayer = layer.Get();
		}

		for (u32 i = 0; i < widget->GetChildCount(); i++)
		{
			if (Ref<FWidget> child = widget->GetChildAt(i))
			{
				SyncWidget(child.Get(), currentLayer, visited);
			}
		}
	}
    
} // namespace Fusion
