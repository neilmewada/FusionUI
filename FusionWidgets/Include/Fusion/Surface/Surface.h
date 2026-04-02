#pragma once

namespace Fusion
{
    class FWidget;
    
    class FUSIONWIDGETS_API FSurface : public FObject
    {
        FUSION_CLASS(FSurface, FObject)
    protected:

        FSurface();

    public:

        // - Getters & Setters -

        FVec2 GetAvailableSize() const { return m_AvailableSize; }

        FVec2 GetPixelSize() const { return m_PixelSize; }

        f32 GetDpiScale() const { return m_DpiScale; }

        Ptr<FWidget> GetRootWidget() const { return m_RootWidget; }

        Ptr<FSurface> GetParentSurface() const { return m_ParentSurface.Lock(); }

        u32 GetChildSurfaceCount() const { return m_ChildSurfaces.Size(); }

        Ptr<FSurface> GetChildSurface(u32 index) const { return m_ChildSurfaces[index]; }

        Ptr<FLayerTree> GetLayerTree() const { return m_LayerTree; }

    public:

        // - Layout -

        void AddPendingLayoutRoot(Ptr<FWidget> layoutRoot);

        void MarkRootLayoutDirty();

        // - Compositing -

        void MarkLayerTreeDirty();

    protected:

        Ptr<FLayerTree> m_LayerTree;

        FHashSet<FUuid> m_PendingLayoutRootIds;
        FArray<Ptr<FWidget>> m_PendingLayoutRoots;

        FArray<Ptr<FSurface>> m_ChildSurfaces;

        WeakPtr<FSurface> m_ParentSurface;

        Ptr<FWidget> m_RootWidget;

        FVec2 m_AvailableSize;

        FVec2 m_PixelSize;

        f32 m_DpiScale = 1.0f;
    };

} // namespace Fusion
