#pragma once

namespace Fusion
{
    
    class FUSIONWIDGETS_API FWidget : public FObject
    {
        FUSION_CLASS(FWidget, FObject)
    public:

        FWidget(FName name = "Widget", Ptr<FObject> outer = nullptr);

    private:

    };

} // namespace Fusion
