#pragma once

namespace Fusion
{
    class FScrollBox;

    FUSION_SIGNAL_TYPE(FScrollBoxSignal, FScrollBox* sender);

    enum class EScrollbarVisibility : u8
    {
        Auto = 0, AlwaysVisible, AlwaysHidden
    };

    class FUSIONWIDGETS_API FScrollBox : public FDecoratedBox
    {
        FUSION_WIDGET(FScrollBox, FDecoratedBox)
    protected:

        FScrollBox();

    public:

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

    protected:

        FUSION_STYLE_PROPERTIES(
            (FBrush, TrackBackground,        Paint),
            (FShape, TrackShape,             Paint),
            (FBrush, ThumbBackground,        Paint),
            (FBrush, ThumbHoverBackground,   Paint),
            (FBrush, ThumbPressedBackground, Paint),
            (FShape, ThumbShape,             Paint),
            (f32,    ScrollbarThickness,     Layout),
            (f32,    ScrollbarPadding,       Layout)
        );

        FUSION_LAYOUT_PROPERTY(FVec2, ScrollOffset);
        FUSION_LAYOUT_PROPERTY(bool, CanScrollVertical);
        FUSION_LAYOUT_PROPERTY(bool, CanScrollHorizontal);
        FUSION_LAYOUT_PROPERTY(EScrollbarVisibility, HorizontalScrollVisibility);
        FUSION_LAYOUT_PROPERTY(EScrollbarVisibility, VerticalScrollVisibility);

    };
    
} // namespace Fusion
