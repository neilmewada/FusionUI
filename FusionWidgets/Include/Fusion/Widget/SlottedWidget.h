#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FSlottedWidget : public FWidget
    {
        FUSION_WIDGET(FSlottedWidget, FWidget)
    protected:

        FSlottedWidget();

        void OnBeforeDestroy() override;

        void Construct() override;

        virtual void OnSlotSet(const FName& slotName) {}

    public:

        FShape GetClipShape() const override { return ClipContent() ? ClipShape() : EShapeType::None; }

        void SetParentSurfaceRecursive(Ref<FSurface> surface) override;

        virtual u32 GetSlotCount() { return 0; }

        virtual bool IsValidSlotWidget(u32 slot, Ref<FWidget> widget) { return false; }

        bool SetSlotWidget(u32 slot, Ref<FWidget> widget);

        int GetChildCount() override;

        Ref<FWidget> GetChildAt(u32 index) override;

        void DetachChild(Ref<FWidget> child) override;

    private:

        TArray<Ref<FWidget>, 2> m_Slots;

    public:

        // - Fusion Properties -

        FUSION_PROPERTY(FShape, ClipShape);
        FUSION_PROPERTY(bool, ClipContent);
    };
    
} // namespace Fusion
