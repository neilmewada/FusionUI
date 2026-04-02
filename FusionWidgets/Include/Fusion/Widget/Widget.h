#pragma once

namespace Fusion
{
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
        FUSION_CLASS(FWidget, FObject)
    public:

        FWidget(FName name = "Widget", Ptr<FObject> outer = nullptr);

        // - Flags -

        virtual void MarkPaintDirty();

        virtual void MarkLayoutDirty();

        bool TestWidgetFlags(EWidgetFlags flags) const { return FEnumHasAllFlags(m_WidgetFlags, flags); }

        bool IsHidden() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Hidden); }

        bool IsExcluded() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::Excluded); }

        bool IsPaintDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::PaintDirty); }

        bool IsLayoutDirty() const { return FEnumHasFlag(m_WidgetFlags, EWidgetFlags::LayoutDirty); }

    protected:

        void OnConstruct() override final;

        virtual void Construct() {}

        virtual void OnPropertyModified(const FName& propertyName);

    public:

        // - Getters -

        Ptr<FWidget> GetParentWidget() const { return m_ParentWidget.Lock(); }

        Ptr<FSurface> GetParentSurface() const { return m_ParentSurface.Lock(); }

        const FAffineTransform& GetCachedLayerSpaceTransform() const { return m_CachedLayerSpaceTransform; }

        FAffineTransform GetGlobalTransform() const;

        FAffineTransform GetChildTransform();

        virtual u32 GetChildCount() const { return 0; }

        virtual Ptr<FWidget> GetChildAt(u32 index) { return nullptr; }

        // - Layout -

        FVec2 GetLayoutPosition() const { return m_LayoutPosition; }

        FVec2 GetLayoutSize() const { return m_LayoutSize; }

        void SetLayoutPosition(FVec2 newPosition);

        FVec2 GetDesiredSize() const { return m_DesiredSize; }

        // - Layer -

        bool IsBoundary() const { return IsCompositingBoundary() || IsPaintBoundary(); }

        bool IsPaintBoundary() const;

        bool IsCompositingBoundary() const;

        void UpdateBoundaryFlags();

    private:

        void SetWidgetFlag(EWidgetFlags flag, bool set);

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

    private:

        // - Internal - 

        WeakPtr<FWidget> m_ParentWidget;

        WeakPtr<FSurface> m_ParentSurface;

        // - Cache -

        FAffineTransform m_CachedLayerSpaceTransform;
        FRect m_CachedLayerSpaceAABB;

        // - Layout - 

        FVec2 m_LayoutPosition;

        FVec2 m_LayoutSize;

        FVec2 m_DesiredSize;

        EWidgetFlags m_WidgetFlags = EWidgetFlags::None;

        FUSION_WIDGET;
    };

} // namespace Fusion
