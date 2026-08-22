#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{

    class FUSIONWIDGETS_API FSurfaceRoot : public FSlottedWidget
    {
        FUSION_WIDGET(FSurfaceRoot, FSlottedWidget)
    protected:

        FSurfaceRoot();

        void Construct() override;

    public:

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        // - Fusion Properties -

        FUSION_SLOTS(
            (FWidget, Content),
			(FContainerWidget, OverlayStack)
        );

    };
    
} // namespace Fusion
