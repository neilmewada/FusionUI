#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FLayerTree : public FObject
    {
        FUSION_CLASS(FLayerTree, FObject)
    public:

        bool IsSyncNeeded() const { return needsSync; }

        void MarkSyncNeeded();

        void DoSyncIfNeeded(FWidget* rootWidget);

        void DoPaintIfNeeded();

        Ptr<FLayer> FindLayerForWidget(FUuid widgetUuid);

        Ptr<FLayer> GetRootLayer() { return rootLayer; }

    protected:

        void SyncWidget(FWidget* widget, Ptr<FLayer> parentLayer, HashSet<FUuid>& visited);

        Ptr<FLayer> rootLayer;

        FAffineTransform rootLayerTransform;

        FHashMap<FUuid, Ptr<FLayer>> widgetUuidToLayerMap;

        bool needsSync = true;
    };
    
} // namespace CE
