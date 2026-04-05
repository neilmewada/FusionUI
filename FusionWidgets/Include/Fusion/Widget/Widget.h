#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    struct FWidgetBuilder
    {
        FWidgetBuilder() {}
    };

    enum class EWidgetFlags
    {
	    None = 0,
        PaintDirty = FUSION_BIT(0),
        LayoutDirty = FUSION_BIT(1),
        Faulted = FUSION_BIT(2),
        ForcePaintBoundary = FUSION_BIT(3),
        ForceCompositingBoundary = FUSION_BIT(4),
        CachedPaintBoundary = FUSION_BIT(5),
        CachedCompositingBoundary = FUSION_BIT(6),
        Hidden = FUSION_BIT(7),
        Excluded = FUSION_BIT(8),
    };
    FUSION_ENUM_CLASS_FLAGS(EWidgetFlags);
    
    class FUSIONWIDGETS_API FWidget : public FObject
    {
        FUSION_WIDGET(FWidget, FObject)
    protected:

        FWidget();

    public:

        ~FWidget();

        // - Flags -

        virtual void MarkPaintDirty();

        virtual void MarkLayoutDirty();

        bool TestWidgetFlags(EWidgetFlags flags) const { return FEnumHasAllFlags(m_WidgetFlags, flags); }

        bool IsHidden() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Hidden); }

        bool IsFaulted() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Faulted); }

        bool IsExcluded() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Excluded); }

        bool IsPaintDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::PaintDirty); }

        bool IsLayoutDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::LayoutDirty); }

        virtual void OnPropertyModified(const FName& propertyName);

    protected:

        void OnConstruct() override final;

        virtual void Construct() {}

    public:

        // - Getters -

        Ref<FApplicationInstance> GetApplication() const;

        Ref<FWidget> GetParentWidget() const { return m_ParentWidget.Lock(); }

        Ref<FSurface> GetParentSurface() const { return m_ParentSurface.Lock(); }

        const FAffineTransform& GetCachedLayerSpaceTransform() const { return m_CachedLayerSpaceTransform; }

        FAffineTransform GetGlobalTransform() const;

        FAffineTransform GetChildTransform();

        virtual u32 GetChildCount() { return 0; }

        virtual Ref<FWidget> GetChildAt(u32 index) { return nullptr; }

        EStyleState GetStyleState() const { return m_StyleState; }

        // - Layout -

        virtual bool IsLayoutBoundary();

        FVec2 GetLayoutPosition() const { return m_LayoutPosition; }

        FVec2 GetLayoutSize() const { return m_LayoutSize; }

        void SetLayoutPosition(FVec2 newPosition);

        FVec2 GetDesiredSize() const { return m_DesiredSize; }

        FVec2 GetMinimumContentSize();

        FVec2 ApplyLayoutConstraints(FVec2 desiredSize);

        virtual FVec2 MeasureContent(FVec2 availableSize);

        virtual void ArrangeContent(FVec2 finalSize);

        // - Style -

        void RefreshStyle();

        void RefreshStyleRecursively();

        Ref<FStyle> ResolveStyle();

        virtual void ApplyStyle(FStyle& style);

        // Hierarchy

        virtual void SetParentSurfaceRecursive(Ref<FSurface> surface);

        virtual void DetachChild(Ref<FWidget> child) {}

        void DetachFromParent();

        virtual void OnAttachedToParent(Ref<FWidget> parent);

        virtual void OnDetachedFromParent(Ref<FWidget> parent);

        // - Layer -

        bool IsBoundary() const { return IsCompositingBoundary() || IsPaintBoundary(); }

        bool IsPaintBoundary() const;

        bool IsCompositingBoundary() const;

        void UpdateBoundaryFlags();

        // - Paint -

        virtual void Paint(FPainter& painter);

        virtual void PaintOverlay(FPainter& painter);

        // - Event -

        virtual void OnMouseEnter(FMouseEvent& event) {}
        virtual void OnMouseLeave(FMouseEvent& event) {}
        virtual FEventReply OnMouseMove(FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseButtonDown(FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseButtonUp(FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseWheel(FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnKeyDown(FKeyEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnKeyUp(FKeyEvent& event) { return FEventReply::Unhandled(); }
        virtual void OnFocusChanged(FFocusEvent& event) {}

        bool SelfHitTest(FVec2 localMousePos);

        // - Internal -

        // For internal use only!
        void SetParentWidget(Ref<FWidget> newParentWidget) { m_ParentWidget = newParentWidget; }

        // For internal use only!
        void SetParentSurface(Ref<FSurface> surface) { m_ParentSurface = surface; }

        // For internal use only!
        void SetFaulted();

    protected:

        // For internal use only!
        void SetWidgetFlag(EWidgetFlags flag, bool set);

        void SetStyleStateFlag(EStyleState state, bool set);

    public: 
    	
    	// - Fusion Properties -

        FUSION_PROPERTY(FAffineTransform, Transform);

        FUSION_LAYOUT_PROPERTY(FMargin, Margin);
        FUSION_LAYOUT_PROPERTY(FMargin, Padding);

        FUSION_LAYOUT_PROPERTY(FVec2, Pivot);

        FUSION_LAYOUT_PROPERTY(f32, MinWidth);
        FUSION_LAYOUT_PROPERTY(f32, MinHeight);

        FUSION_LAYOUT_PROPERTY(f32, MaxWidth);
        FUSION_LAYOUT_PROPERTY(f32, MaxHeight);

        FUSION_LAYOUT_PROPERTY(f32, FillRatio);

        FUSION_LAYOUT_PROPERTY(f32, Opacity);

        FUSION_PROPERTY(FShape, ClipShape);
        FUSION_PROPERTY(bool, ClipContent);

        FUSION_PROPERTY(FName, Style);

        FUSION_LAYOUT_PROPERTY(EHAlign, HAlign);
        FUSION_LAYOUT_PROPERTY(EVAlign, VAlign);

        FUSION_PROPERTY_SET(f32, Width)
        {
            return self
                .MinWidth(value)
                .MaxWidth(value);
        }

        FUSION_PROPERTY_SET(f32, Height)
        {
            return self
                .MinHeight(value)
                .MaxHeight(value);
        }

        FUSION_PROPERTY_GET(bool, Excluded)
        {
            return IsExcluded();
        }

        FUSION_PROPERTY_GET(bool, Visible)
        {
            return !IsHidden();
        }

        template<typename TWidget> requires TFIsDerivedClass<FWidget, TWidget>::Value
        TWidget& Assign(TWidget*& out)
        {
            out = (TWidget*)this;
            return *out;
        }

        template<typename TWidget> requires TFIsDerivedClass<FWidget, TWidget>::Value
        TWidget& Assign(Ref<TWidget>& out)
        {
            out = (TWidget*)this;
            return *out;
        }

        FUSION_PROPERTY_SET(FString, Name)
        {
            self.SetName(value);
            return self;
        }

    protected:

        // - Layout - 

        FVec2 m_LayoutPosition;

        FVec2 m_LayoutSize;

        FVec2 m_DesiredSize;

    private:

        // - Internal - 

        WeakRef<FWidget> m_ParentWidget;

        WeakRef<FSurface> m_ParentSurface;

        // - Cache -

        FAffineTransform m_CachedLayerSpaceTransform;
        FRect m_CachedLayerSpaceAABB;

        EWidgetFlags m_WidgetFlags = EWidgetFlags::None;
        EStyleState m_StyleState = EStyleState::Default;

        friend class FLayer;
        friend class FSurface;
    };

    template<class T>
    concept FWidgetClassType = TFIsDerivedClass<FWidget, T>::Value;

} // namespace Fusion
