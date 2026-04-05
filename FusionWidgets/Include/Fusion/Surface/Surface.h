#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FWidget;
    
    class FUSIONWIDGETS_API FSurface : public FObject
    {
        FUSION_CLASS(FSurface, FObject)
    protected:

        FSurface(FName name = "Surface");

    public:

        // - Getters -

        FVec2 GetAvailableSize() const { return m_AvailableSize; }

        FVec2 GetPixelSize() const { return m_PixelSize; }

        f32 GetDpiScale() const { return m_DpiScale; }

        Ref<FWidget> GetRootWidget() const { return m_RootWidget; }

        Ref<FSurface> GetParentSurface() const { return m_ParentSurface.Lock(); }

        u32 GetChildSurfaceCount() const { return m_ChildSurfaces.Size(); }

        Ref<FSurface> GetChildSurface(u32 index) const { return m_ChildSurfaces[index]; }

        Ref<FLayerTree> GetLayerTree() const { return m_LayerTree; }

        Ref<FApplicationInstance> GetApplication() const { return m_Application.Lock(); }

        virtual bool IsNativeSurface() const { return false; }

        Ref<FStyleSheet> GetStyleSheet() const;

        FWidget* HitTestWidget(FVec2 pos, FWidget* widget = nullptr);

        // - Coordinate Transforms -

        virtual FVec2 ScreenToSurfacePoint(FVec2 position) = 0;

        // - Events -

        virtual void DispatchSurfaceUnfocusEvent();

        virtual void DispatchSurfaceFocusEvent();

        virtual void DispatchMouseEvents();
        virtual void DispatchKeyEvents();

        void ProcessReply(Ref<FWidget> sender, const FEventReply& reply);

        // - Lifecycle -

        virtual void Initialize();

        virtual void Shutdown();

        virtual void TickSurface();

        virtual void OnSurfaceResize();

    public:

        // - Widget -

        void SetOwningWidget(Ref<FWidget> widget);

        // - Layout -

        void AddPendingLayoutRoot(Ref<FWidget> layoutRoot);

        void MarkRootLayoutDirty();

        // - Compositing -

        void MarkLayerTreeDirty();

        void CompositeLayer(IntrusivePtr<FRenderSnapshot> snapshot, Ref<FLayer> layer, int layerIndex);

    protected:

        Ref<FLayerTree> m_LayerTree;

        FHashSet<FUuid> m_PendingLayoutRootIds;
        FArray<Ref<FWidget>> m_PendingLayoutRoots;

        FArray<Ref<FSurface>> m_ChildSurfaces;

        WeakRef<FSurface> m_ParentSurface;

        WeakRef<FApplicationInstance> m_Application;

        Ref<FWidget> m_RootWidget;

        FRenderBackendCapabilities m_RenderCapabilities{};

        FRenderTargetHandle m_RenderTarget;

        FVec2 m_AvailableSize;

        FVec2 m_PixelSize;

        f32 m_DpiScale = 1.0f;

        // - Style -

        Ref<FStyleSheet> m_StyleSheet;

        // - Event -

        FArray<WeakRef<FWidget>> hoveredWidgetStack;
        std::array<WeakRef<FWidget>, 5> pressedWidgetPerButton;
        WeakRef<FWidget> curFocusedWidget, nextFocusWidget;
        WeakRef<FWidget> capturedWidget;

        friend class FApplicationInstance;
    };

} // namespace Fusion
