#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

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

        FUSION_PROPERTY_GET(f32, FontSize)
        {
            return Font().GetPointSize();
        }

        FUSION_PROPERTY_SET(f32, FontSize)
        {
            self.Font(self.Font().PointSize(value));
            return self;
        }

    };

} // namespace Fusion
