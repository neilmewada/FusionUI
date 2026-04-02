#pragma once

namespace Fusion
{
    class FLayerTree;
    class FPainter;

    class FUSIONWIDGETS_API FLayer : public FObject
    {
        FUSION_CLASS(FLayer, FObject)
    public:

        FLayer(FName name, FObject* outer = nullptr);

        Ptr<FWidget> GetOwningWidget() { return owningWidget.Lock(); }

        Ptr<FLayer> GetParentLayer() { return parent.Lock(); }

        f32 GetDpiScale();

        bool NeedsCompositing() { return needsCompositing; }

        bool NeedsRepaint();

        void DoPaintIfNeeded();

        u32 GetChildCount() { return (u32)children.Size(); }

        Ptr<FLayer> GetChild(u32 index) { return children[index]; }

        //FUIDrawList* GetDrawList() { return &drawList; }

        u32 GetSplitPointCount() { return splitPoints.Size(); }

        u32 GetSplitPoint(u32 index) { return splitPoints[index]; }

        const FAffineTransform& GetTransformInParentSpace() const { return cachedTransformInParentLayerSpace; }

        FAffineTransform GetGlobalTransform();

    protected:

        void DoPaint();

        void DoPaint(FWidget* widget, FPainter& painter);

        WeakPtr<FLayerTree> ownerTree;

        WeakPtr<FLayer> parent;

        WeakPtr<FWidget> owningWidget;

        FArray<Ptr<FLayer>> children;

        FAffineTransform cachedTransformInParentLayerSpace;

        bool needsCompositing = false;
        //FUIDrawList drawList;
        FArray<u32> splitPoints;

        friend class FLayerTree;
        friend class FPainter;
        friend class FRenderSnapshot;
    };

} // namespace CE
