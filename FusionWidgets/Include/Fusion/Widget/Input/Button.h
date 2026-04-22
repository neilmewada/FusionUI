#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FButton;

    FUSION_SIGNAL_TYPE(FButtonSignal, FButton* button);


    class FUSIONVULKANRHI_API FButton : public FDecoratedBox
    {
        FUSION_CLASS(FButton, FDecoratedBox)
    public:

        FButton();

    protected:

        void Construct() override;

        // - Events -

        void OnMouseEnter(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;

        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;

        FEventReply OnKeyDown(FKeyEvent& event) override;
        FEventReply OnKeyUp(FKeyEvent& event) override;

        void OnFocusChanged(FFocusEvent& event) override;

    public:

        FUSION_SIGNAL(FButtonSignal, OnClick);
        FUSION_SIGNAL(FButtonSignal, OnDoubleClick);
    };
    
} // namespace Fusion
