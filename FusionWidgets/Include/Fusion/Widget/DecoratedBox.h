#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONWIDGETS_API FDecoratedBox : public FCompoundWidget
    {
        FUSION_CLASS(FDecoratedBox, FCompoundWidget)
    public:

        FDecoratedBox();

    protected:

        void Paint(FPainter& painter) override;

        void PaintOverContent(FPainter& painter) override;

    public:

        // - Fusion Properties -

        FUSION_STYLE_PROPERTIES(
            (FBrush, Background,      Paint),
            (FPen,   Border,          Paint),
            (FPen,   Outline,         Paint),
            (f32,    OutlineOffset,   Paint),
            (FShape, Shape,           Paint)
        );

    };

} // namespace Fusion
