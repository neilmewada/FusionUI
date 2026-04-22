#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    struct FWidgetBuilder
    {
        FWidgetBuilder() {}
    };

    enum class EWidgetFlags : u32
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
        Focusable = FUSION_BIT(9),

        // If the widget is still in the Construct() method.
        PendingConstruction = FUSION_BIT(10),

        StyleScopeBoundary = FUSION_BIT(11),
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

        bool IsHidden()    const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Hidden); }

        bool IsFaulted()   const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Faulted); }

        bool IsExcluded()  const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Excluded); }

        bool IsFocusable() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Focusable); }

        bool IsPaintDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::PaintDirty); }

        bool IsLayoutDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::LayoutDirty); }

        bool IsWidgetPendingConstruction() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::PendingConstruction); }

        bool IsStyleScopeBoundary() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::StyleScopeBoundary); }

        virtual void OnPropertyModified(const FName& propertyName);

    protected:

        void OnConstruct() override final;

        virtual void Construct() {}

    public:

        // - Getters -

        Ref<FApplicationInstance> GetApplication() const;

        Ref<FWidget> GetParentWidget() const { return m_ParentWidget.Lock(); }

        Ref<FSurface> GetParentSurface() const { return m_ParentSurface.Lock(); }

        //! @brief Returns the cached layer space transform, which can be used to transform local position to Layer space position.
        const FAffineTransform& GetCachedLayerSpaceTransform() const { return m_CachedLayerSpaceTransform; }

        //! @brief Computes and returns the transform that can be used to transform local position to Global (i.e. surface space) position.
        FAffineTransform GetGlobalTransform() const;

        FAffineTransform GetChildTransform();

        virtual int GetChildCount() { return 0; }

        virtual Ref<FWidget> GetChildAt(u32 index) { return nullptr; }

        int GetIndexOfChild(Ref<FWidget> child);

        EStyleState GetStyleState();

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

        void ArrangeContentBase(FVec2 finalSize);

        // - Style -

        //! @brief Clears the cached style, resolves the style again, and then applies it. Use sparingly, and prefer using ApplyStyle() over this.
        void RefreshStyle();

        //! @brief Applied the already cached style if it exists, otherwise it resolves and then applies it.
        void ApplyStyle();

        void RefreshStyleRecursively();

        FName ResolveStyleName();

        Ref<FStyle> ResolveStyle();

        inline bool TestStyleState(EStyleState state) const { return FEnumHasAllFlags(m_StyleState, state); }

        // Hierarchy

        virtual void SetParentSurfaceRecursive(Ref<FSurface> surface);

        void DetachFromParent();

        virtual void DetachChild(Ref<FWidget> child) {}

        virtual void OnAttachedToParent(Ref<FWidget> parent);

        virtual void OnDetachedFromParent(Ref<FWidget> parent);

        Ref<FWidget> FindSubWidgetTypeInHierarchy(FTypeID widgetClassId);

        template<class TWidget>
        Ref<FWidget> FindSubWidgetTypeInHierarchy()
        {
            return FindSubWidgetTypeInHierarchy(TWidget::StaticClassTypeID());
        }

        // - Layer -

        bool IsBoundary() const { return IsCompositingBoundary() || IsPaintBoundary(); }

        bool IsPaintBoundary() const;

        bool IsCompositingBoundary() const;

        void UpdateBoundaryFlags();

        // - Cursor -

        virtual FCursor GetActiveCursorAt(FVec2 localPos) { return FCursor::Inherit(); }

        // - Paint -

        virtual void Paint(FPainter& painter);
        virtual void PaintOverContent(FPainter& painter);

        // - Clip -

        virtual FShape GetClipShape() const { return EShapeType::None; }

        bool IsClipContent() const { return GetClipShape().GetShapeType() != EShapeType::None; }

        // - Event -

        virtual void OnMouseEnter([[maybe_unused]] FMouseEvent& event) {}
        virtual void OnMouseLeave([[maybe_unused]] FMouseEvent& event) {}
        virtual FEventReply OnMouseMove([[maybe_unused]] FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseButtonDown([[maybe_unused]] FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseButtonUp([[maybe_unused]] FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnMouseWheel([[maybe_unused]] FMouseEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnKeyDown([[maybe_unused]] FKeyEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnKeyUp([[maybe_unused]] FKeyEvent& event) { return FEventReply::Unhandled(); }
        virtual FEventReply OnTextInput([[maybe_unused]] FTextInputEvent& event) { return FEventReply::Unhandled(); }
        virtual void OnFocusChanged([[maybe_unused]] FFocusEvent& event) {}

        virtual void OnEnabled() {}
        virtual void OnDisabled() {}

        virtual bool ShouldHitTestChildren(FVec2 localMousePos) { return true; }

        bool SelfHitTest(FVec2 localMousePos);

        // - Internal -

        // For internal use only!
        void SetParentWidget(Ref<FWidget> newParentWidget) { m_ParentWidget = newParentWidget; }

        // For internal use only!
        void SetParentSurface(Ref<FSurface> surface) { m_ParentSurface = surface; }

        // For internal use only!
        void SetFaulted();

    protected:

        void AttachChildWidget(Ref<FWidget> child);
        void DetachChildWidget(Ref<FWidget> child);

        // For internal use only!
        void SetWidgetFlag(EWidgetFlags flag, bool set);

        void SetStyleStateFlag(EStyleState state, bool set);

    public: 
    	
    	// - Fusion Properties -

        //FUSION_PROPERTY(FAffineTransform, Transform);

        FUSION_LAYOUT_PROPERTY(FMargin, Margin);

        __FUSION_STYLE_PROPERTIES_FWIDGET(
            (FMargin,          Padding,    Layout),
            (FAffineTransform, Transform,  Layout)
        );

        FUSION_LAYOUT_PROPERTY(FVec2, Pivot);

        FUSION_LAYOUT_PROPERTY(f32, MinWidth);
        FUSION_LAYOUT_PROPERTY(f32, MinHeight);

        FUSION_LAYOUT_PROPERTY(f32, MaxWidth);
        FUSION_LAYOUT_PROPERTY(f32, MaxHeight);

        FUSION_LAYOUT_PROPERTY(f32, FillRatio);

        FUSION_PROPERTY(f32, Opacity);

        FUSION_PROPERTY(FName, Style);
        FUSION_PROPERTY(FName, SubStyle);
        FUSION_PROPERTY(EStyleState, PropagatedStyleStates);

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

        FUSION_PROPERTY_SET(bool, Excluded)
        {
            static_cast<FWidget&>(self).SetWidgetFlag(EWidgetFlags::Excluded, value);
            return self;
        }

        FUSION_PROPERTY_GET(bool, Visible)
        {
            return !IsHidden();
        }

        FUSION_PROPERTY_GET(bool, Enabled)
        {
            return !TestStyleState(EStyleState::Disabled);
        }

        FUSION_PROPERTY_SET(bool, Enabled)
        {
            static_cast<FWidget&>(self).SetEnabledRecursive(value, self.GetParentWidget());
            return self;
        }

        FUSION_PROPERTY_SET(bool, StyleScopeBoundary)
        {
            static_cast<FWidget&>(self).SetWidgetFlag(EWidgetFlags::StyleScopeBoundary, value);
            return self;
        }

        void SetEnabledRecursive(bool enabled, Ref<FWidget> parent = nullptr);

        bool IsAncestorDisabled();
        bool IsAncestorExcluded();
        bool IsAncestorHidden();

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
        Ref<FStyle> m_CachedStyle;
        bool m_StyleCached = false;

        EWidgetFlags m_WidgetFlags = EWidgetFlags::None;
        EStyleState m_StyleState = EStyleState::Default;

        friend class FLayer;
        friend class FSurface;
    };

} // namespace Fusion
