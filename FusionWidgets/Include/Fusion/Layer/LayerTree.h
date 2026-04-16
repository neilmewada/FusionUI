#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FLayerTree : public FObject
    {
        FUSION_CLASS(FLayerTree, FObject)
    public:

        FLayerTree();

        bool IsSyncNeeded() const { return m_NeedsSync; }

        void MarkSyncNeeded();

        void DoSyncIfNeeded(FWidget* rootWidget);

        void DoPaintIfNeeded();

        Ref<FLayer> FindLayerForWidget(FUuid widgetUuid);

        Ref<FLayer> GetRootLayer() { return m_RootLayer; }

        Ref<FLayer> GetOverlayLayer() { return m_OverlayLayer; }

    protected:

        void SyncWidget(Ref<FWidget> widget, Ref<FLayer> parentLayer, FHashSet<FUuid>& visited);

        Ref<FLayer> m_RootLayer;
        Ref<FLayer> m_OverlayLayer;

        FAffineTransform m_RootLayerTransform;

        FHashMap<FUuid, Ref<FLayer>> m_WidgetUuidToLayerMap;

        bool m_NeedsSync = true;
    };
    
} // namespace CE
