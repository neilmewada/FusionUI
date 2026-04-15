#pragma once

namespace Fusion
{
    
    class FUSIONWIDGETS_API FDecoratedWidget : public FCompoundWidget
    {
        FUSION_CLASS(FDecoratedWidget, FCompoundWidget)
    public:

        FDecoratedWidget();

    protected:

        void Paint(FPainter& painter) override;

    public:

        // - Fusion Properties -

        FUSION_STYLE_PROPERTIES(
            (FBrush, Background, Paint),
            (FPen,   Border,     Paint),
            (FPen,   Outline,    Paint),
            (FShape, Shape,      Paint)
        );

    };

} // namespace Fusion
