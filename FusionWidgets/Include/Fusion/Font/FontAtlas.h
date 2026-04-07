#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FFontAtlas : public FObject
    {
        FUSION_CLASS(FFontAtlas, FObject)
    public:

        FFontAtlas() = default;

        

    private:

        struct PImpl* pImpl = nullptr;
    };
    
} // namespace Fusion
