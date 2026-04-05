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

        void ApplyStyle(FStyle& style) override;

    public: // - Fusion Properties - 

        FUSION_STYLE_PROPERTY(FBrush, Background);
        FUSION_STYLE_PROPERTY(FPen, Border);
        FUSION_STYLE_PROPERTY(FShape, Shape);

    };

} // namespace Fusion
