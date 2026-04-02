#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include <type_traits>
#include <concepts>

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

    template<class TBase, class TDerived>
    using TFIsDerivedClass = TFBoolConst<__is_base_of(TBase, TDerived)>;

    template<typename T>
    struct TFIsTemplate : TFFalseType {};

    template<typename T, template <typename> class U>
    struct TFIsTemplate<U<T>> : TFTrueType {};

    template<typename T, typename = void>
    struct TFHasGetHashFunction : TFFalseType
    {
        static SizeT GetHash(const T* instance) { return 0; }
    };

    template<typename T>
    struct TFHasGetHashFunction<T, std::void_t<decltype(std::declval<T>().GetHash())>> : TFTrueType
    {
        static SizeT GetHash(const T* instance) { return instance->GetHash(); }
    };

    template<typename T>
    constexpr std::remove_reference_t<T>&& MoveTemp(T&& value)
    {
        return static_cast<std::remove_reference_t<T>&&>(value);
    }

    template<typename T, size_t InlineCapacity>
    class FArray;

    template<typename T>
	struct TFIsArray : TFFalseType {};

    template<typename T, size_t InlineCapacity>
    struct TFIsArray<FArray<T, InlineCapacity>> : TFTrueType {};

    template<typename T>
    class Ptr;

    template<typename T>
    class WeakPtr;

    template<typename T>
    struct TFIsPtr : TFFalseType
    {
		typedef void Type;
    };

    template<typename T>
    struct TFIsPtr<Ptr<T>> : TFTrueType
    {
        typedef T Type;
    };

    template<typename T>
    struct TFIsWeakPtr : TFFalseType
    {
        typedef void Type;
    };

    template<typename T>
    struct TFIsWeakPtr<WeakPtr<T>> : TFTrueType
    {
        typedef T Type;
    };

    template<typename T>
    struct TFIsIntegralType : TFFalseType {};

    template<>
    struct TFIsIntegralType<u8> : TFTrueType {};
    template<>
    struct TFIsIntegralType<u16> : TFTrueType {};
    template<>
    struct TFIsIntegralType<u32> : TFTrueType {};
    template<>
    struct TFIsIntegralType<u64> : TFTrueType {};
    template<>
    struct TFIsIntegralType<i8> : TFTrueType {};
    template<>
    struct TFIsIntegralType<i16> : TFTrueType {};
    template<>
    struct TFIsIntegralType<i32> : TFTrueType {};
    template<>
    struct TFIsIntegralType<i64> : TFTrueType {};

    class FObject;

    template<class TObject>
    concept FObjectType = TFIsDerivedClass<FObject, TObject>::Value;

    template <typename T>
    struct TFEquitable : TFBoolConst<std::equality_comparable<T>>
    {
        static bool AreEqual(const T& lhs, const T& rhs)
        {
            return lhs == rhs;
        }
    };
    
} // namespace Fusion
