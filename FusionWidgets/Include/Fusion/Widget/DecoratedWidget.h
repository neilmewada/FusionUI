#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FDecoratedWidget : public FSlottedWidget
    {
        FUSION_CLASS(FDecoratedWidget, FSlottedWidget)
    protected:

        FDecoratedWidget();

        void Paint(FPainter& painter) override;

        void PaintOverContent(FPainter& painter) override;

    public:

        // - Public API -

        u32 GetSlotCount() override { return 0; }

        bool IsValidSlotWidget(u32 slot, Ref<FWidget> widget) override { return false; }

    public:

        // - Fusion Properties -

        FUSION_STYLE_PROPERTIES(
            (FBrush, Background,    Paint),
            (FPen,   Border,        Paint),
            (FPen,   Outline,       Paint),
            (f32,    OutlineOffset, Paint),
            (FShape, Shape,         Paint)
        );

    };

} // namespace Fusion
