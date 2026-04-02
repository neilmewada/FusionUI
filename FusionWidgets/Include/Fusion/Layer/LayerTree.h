#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FLayerTree : public FObject
    {
        FUSION_CLASS(FLayerTree, FObject)
    public:

        FLayerTree() : Super("LayerTree")
		{}

        bool IsSyncNeeded() const { return m_NeedsSync; }

        void MarkSyncNeeded();

        void DoSyncIfNeeded(FWidget* rootWidget);

        void DoPaintIfNeeded();

        Ptr<FLayer> FindLayerForWidget(FUuid widgetUuid);

        Ptr<FLayer> GetRootLayer() { return m_RootLayer; }

    protected:

        void SyncWidget(Ptr<FWidget> widget, Ptr<FLayer> parentLayer, FHashSet<FUuid>& visited);

        Ptr<FLayer> m_RootLayer;

        FAffineTransform m_RootLayerTransform;

        FHashMap<FUuid, Ptr<FLayer>> m_WidgetUuidToLayerMap;

        bool m_NeedsSync = true;
    };
    
} // namespace CE
