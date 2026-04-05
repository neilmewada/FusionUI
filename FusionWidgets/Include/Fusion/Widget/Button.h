#pragma once

namespace Fusion
{
    class FButton;

    FUSION_SIGNAL_TYPE(FButtonSignal, FButton* button);


    class FUSIONVULKANRHI_API FButton : public FDecoratedWidget
    {
        FUSION_CLASS(FButton, FDecoratedWidget)
    public:

        FButton();

    protected:

        // - Events -

        void OnMouseEnter(FMouseEvent& event) override;
        void OnMouseLeave(FMouseEvent& event) override;

        FEventReply OnMouseButtonDown(FMouseEvent& event) override;
        FEventReply OnMouseButtonUp(FMouseEvent& event) override;

    public:

        FUSION_SIGNAL(FButtonSignal, OnClick);

    private:

        bool m_Pressed = false;
    };
    
} // namespace Fusion
