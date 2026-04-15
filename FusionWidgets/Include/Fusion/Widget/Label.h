#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FLabel : public FWidget
    {
        FUSION_WIDGET(FLabel, FWidget)
    protected:

        FLabel();

    public:

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        void Paint(FPainter& painter) override;

    protected:

        FUSION_LAYOUT_PROPERTY(FString, Text);

        FUSION_STYLE_PROPERTIES(
            (FFont,   Font,  LayoutAndPaint),
            (FColor,  Color, Paint)
        );

    };

} // namespace Fusion
