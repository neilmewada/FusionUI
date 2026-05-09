#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FItemView;

    struct FItemViewCellInfo
    {
        FRect  Rect;
        bool   IsExpandable = false;
        bool   IsExpanded   = false;
        f32    LeftPadding  = 0.0f;
        f32    ChevronSize  = 14.0f, ChevronGap = 0.0f;
        f32    IconWidth    = 14.0f, IconGap    = 0.0f;
        FColor ChevronColor = FColors::White;
    };

    struct FItemViewLayout
    {
        FRect ChevronRect;
        FRect IconRect;
    };

    class FUSIONWIDGETS_API FItemViewDelegate : public FObject
    {
        FUSION_CLASS(FItemViewDelegate, FObject)
    protected:

        FItemViewDelegate() = default;

    public:

        virtual FItemViewLayout Paint(FPainter& painter, FModelIndex index, const FItemViewCellInfo& info);

    private:

    };

} // namespace Fusion
