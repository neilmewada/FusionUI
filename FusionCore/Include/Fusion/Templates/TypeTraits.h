#pragma once

#include <type_traits>

namespace Fusion
{

    template<bool TValue>
    struct TFBoolConst
    {
        static constexpr bool Value = TValue;
    };

    struct TFFalseType : TFBoolConst<false> {};

    struct TFTrueType : TFBoolConst<true> {};

    template<typename T1, typename T2>
    struct TFIsSameType : TFFalseType {};

    template<typename T>
    struct TFIsSameType<T, T> : TFTrueType {};

    template<typename T>
    struct TFIsTemplate : TFFalseType {};

    template<typename T, template <typename> class U>
    struct TFIsTemplate<U<T>> : TFTrueType {};
    
} // namespace Fusion
