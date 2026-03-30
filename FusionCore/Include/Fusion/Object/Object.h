#pragma once

#include "Fusion/Object/RefCountBlock.h"
#include <atomic>

namespace Fusion
{
    template<typename T> class Ptr;
    template<typename T> class WeakPtr;

    class FUSIONCORE_API FObject
    {
    public:
        FObject();
        virtual ~FObject();

        FObject(const FObject&)            = delete;
        FObject& operator=(const FObject&) = delete;

    protected:
        virtual void OnDestroy() {}

    private:
        std::atomic<Internal::RefCountBlock*> m_Control = nullptr;

        template<typename T> friend class Ptr;
        template<typename T> friend class WeakPtr;
        friend struct Internal::RefCountBlock;
    };

} // namespace Fusion
