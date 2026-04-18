#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FTextButton : public FButton
    {
        FUSION_WIDGET(FTextButton, FButton)
    protected:

        FTextButton();

        void Construct() override;

    public:

    protected:

        Ref<FLabel> m_Label;

    public: // - Properties -

        FUSION_PROPERTY_FORWARD(FString, Text,      m_Label, Text);
        FUSION_PROPERTY_FORWARD(FFont,   Font,      m_Label, Font);
        FUSION_PROPERTY_FORWARD(FColor,  TextColor, m_Label, Color);

    };
    
} // namespace Fusion
