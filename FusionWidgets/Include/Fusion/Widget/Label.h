#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FLabel : public FWidget
    {
        FUSION_WIDGET(FLabel, FWidget)
    protected:

        FLabel();

    public:


    protected:

        FUSION_LAYOUT_PROPERTY(FString, Text);

        FUSION_STYLE_PROPERTIES(
			(FFont, Font, LayoutAndPaint)
        );

    };
    
} // namespace Fusion
