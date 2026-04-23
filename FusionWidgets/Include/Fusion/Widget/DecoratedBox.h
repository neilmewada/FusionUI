#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    
    class FUSIONWIDGETS_API FDecoratedBox : public FDecoratedWidget
    {
        FUSION_CLASS(FDecoratedBox, FDecoratedWidget)
    protected:

        FDecoratedBox();

    public:

        Ref<FWidget> GetChild() const { return m_Child; }

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

    public:

        // - Fusion Properties -

        FUSION_SLOTS(
            (FWidget, Child)
        );

    private:

        FVec4 m_InternalPadding;
    };

} // namespace Fusion
