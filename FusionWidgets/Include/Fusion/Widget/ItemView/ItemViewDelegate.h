#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FItemView;

    struct FItemViewPaintInfo
    {
        Ref<FItemView> View;
        Ref<FItemModel> Model;
        FRect Rect;
    };

    class FUSIONWIDGETS_API FItemViewDelegate : public FObject
    {
        FUSION_CLASS(FItemViewDelegate, FObject)
    protected:

        FItemViewDelegate() = default;

    public:

        virtual void Paint(FPainter& painter, FModelIndex index, const FItemViewPaintInfo& info);

    private:

    };

} // namespace Fusion
