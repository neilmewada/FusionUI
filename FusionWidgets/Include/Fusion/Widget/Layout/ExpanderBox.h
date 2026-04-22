#pragma once

namespace Fusion
{
    
    class FUSIONWIDGETS_API FExpanderBox : public FSlottedWidget
    {
        FUSION_WIDGET(FExpanderBox, FSlottedWidget)
    protected:
        
        FExpanderBox();

        void Construct() override;

        // - Layout -

        FVec2 MeasureContent(FVec2 availableSize) override;

        void ArrangeContent(FVec2 finalSize) override;

        // - Paint -

        void Paint(FPainter& painter) override;

    public:

        static constexpr f32 kExpansionThreshold = 0.0001f;
        
        bool IsExpanded() const
        {
            return TestStyleState(EStyleState::Expanded);
        }

        void SetExpanded(bool expanded);

    protected:

        Ref<FWidget> m_Child;
        Ref<FLabel> m_TitleLabel;

        void SetupHeader();
        void SetupContent();

        void OnSlotSet(const FName& slotName) override;

    public:

        // - Fusion Properties -

        FUSION_SLOTS(
			(FButton, Header),
			(FDecoratedBox, Content)
        )

        FUSION_PROPERTY_SET(FWidget&, Child)
        {
            if (self.m_Child == &value)
                return self;
            self.m_Child = &value;
            self.m_Content->Child(value);
            static_cast<FExpanderBox&>(self).SetupContent();
            return self;
        }

        FUSION_PROPERTY_GET(bool, Expanded)
        {
            return IsExpanded();
        }

        FUSION_PROPERTY_SET(bool, Expanded)
        {
            self.SetExpanded(value);
            return self;
        }

        FUSION_STYLE_PROPERTIES(
            (FBrush, Background, Paint),
            (FPen,   Border,     Paint),
            (FShape, Shape,      Paint),
			(f32,    ExpandedAmount,   Layout)
        );

        FUSION_PROPERTY_FORWARD(FString, Title, m_TitleLabel, Text);
        
    };

} // namespace Fusion
