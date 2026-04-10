#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FLayerTree;
    class FPainter;

    class FUSIONWIDGETS_API FLayer : public FObject
    {
        FUSION_CLASS(FLayer, FObject)
    public:

        FLayer(FName name = "Layer");

        Ref<FWidget> GetOwningWidget() { return m_OwningWidget.Lock(); }

        Ref<FSurface> GetWidgetSurface();

        Ref<FLayer> GetParentLayer() { return m_Parent.Lock(); }

        f32 GetDpiScale();

        bool NeedsCompositing() { return m_NeedsCompositing; }

        bool NeedsRepaint();

        void DoPaintIfNeeded();

        u32 GetChildCount() { return (u32)m_Children.Size(); }

        Ref<FLayer> GetChild(u32 index) { return m_Children[index]; }

        FUIDrawList* GetDrawList() { return &m_DrawList; }

        u32 GetSplitPointCount() { return m_SplitPoints.Size(); }

        SizeT GetSplitPoint(u32 index) { return m_SplitPoints[index]; }

        const FAffineTransform& GetTransformInParentSpace() const { return m_CachedTransformInParentLayerSpace; }

        FAffineTransform GetGlobalTransform();

    protected:

        void DoPaint();

        void DoPaint(Ref<FWidget> widget, FPainter& painter);

        WeakRef<FLayerTree> m_OwnerTree;

        WeakRef<FLayer> m_Parent;

        WeakRef<FWidget> m_OwningWidget;

        FArray<Ref<FLayer>> m_Children;

        FAffineTransform m_CachedTransformInParentLayerSpace;

        bool m_NeedsCompositing = false;
        FUIDrawList m_DrawList;
        FArray<SizeT> m_SplitPoints;

        friend class FLayerTree;
        friend class FPainter;
        friend class FSurface;
    };

} // namespace CE
