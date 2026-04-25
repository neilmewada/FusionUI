#pragma once

namespace Fusion
{

    class FUSIONWIDGETS_API FTextDocument
    {
    public:

        FTextDocument();

    private:
        TUniquePtr<FLineBuffer> m_LineBuffer;
        
    };
    
} // namespace Fusion
