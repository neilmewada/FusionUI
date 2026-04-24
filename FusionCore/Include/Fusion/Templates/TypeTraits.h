#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include <type_traits>
#include <concepts>
#include <tuple>

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
    class TArray;

    template<typename T>
	struct TFIsArray : TFFalseType {};

    template<typename T, size_t InlineCapacity>
    struct TFIsArray<TArray<T, InlineCapacity>> : TFTrueType {};

    template<typename T>
    class Ref;

    template<typename T>
    class WeakRef;

    template<typename T>
    struct TFIsPtr : TFFalseType
    {
		typedef void Type;
    };

    template<typename T>
    struct TFIsPtr<Ref<T>> : TFTrueType
    {
        typedef T Type;
    };

    template<typename T>
    struct TFIsWeakPtr : TFFalseType
    {
        typedef void Type;
    };

    template<typename T>
    struct TFIsWeakPtr<WeakRef<T>> : TFTrueType
    {
        typedef T Type;
    };

    template<typename T>
    struct TPtrType : TFFalseType
    {
        typedef void Type;

        static T* GetRawPtr(T ptr) { return nullptr; }
    };

    template<typename T>
    struct TPtrType<T*> : TFTrueType
    {
        typedef T Type;

        static T* GetRawPtr(T* ptr) { return ptr; }
    };

    template<typename T>
    struct TPtrType<Ref<T>> : TFTrueType
    {
        typedef T Type;

        static T* GetRawPtr(Ref<T> ptr);
    };

    template<typename T>
    struct TPtrType<WeakRef<T>> : TFTrueType
    {
        typedef T Type;

        static T* GetRawPtr(WeakRef<T> ptr);
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

    // Function

    template <typename T, typename = void>
    struct TFunctionTraits
    {
        static constexpr bool Value = false;
    };

    template <typename T>
    struct TFunctionTraits<T, std::void_t<decltype(&T::operator())>> : TFunctionTraits<decltype(&T::operator())>
    {
    };

    // For generic types, directly use the result of the signature of its 'operator()'

    template <typename TClassType, typename TReturnType, typename... Args>
    struct TFunctionTraits<TReturnType(TClassType::*)(Args...) const> // we specialize for pointers to const member function
    {
        enum { NumArgs = sizeof...(Args) };

        static constexpr bool Value = true;

        typedef TReturnType ReturnType;
        typedef TClassType ClassType;

        typedef TReturnType(TClassType::* FuncSignature)(Args...) const;

        typedef std::tuple<Args...> Tuple;

        template <SizeT i>
        struct Arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type Type;
            // the i-th argument is equivalent to the i-th tuple element of a tuple
            // composed of those arguments.
        };
    };

    template <typename TClassType, typename TReturnType, typename... Args>
    struct TFunctionTraits<TReturnType(TClassType::*)(Args...)> // we specialize for pointers to member function
    {
        enum { NumArgs = sizeof...(Args) };

        static constexpr bool Value = true;

        typedef TReturnType ReturnType;
        typedef TClassType ClassType;

        typedef TReturnType(TClassType::* FuncSignature)(Args...);

        typedef std::tuple<Args...> Tuple;

        template <SizeT i>
        struct Arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type Type;
            // the i-th argument is equivalent to the i-th tuple element of a tuple
            // composed of those arguments.
        };
    };

    template <typename TReturnType, typename... Args>
    struct TFunctionTraits<TReturnType(*)(Args...)> // we specialize for pointers to global functions
    {
        enum { NumArgs = sizeof...(Args) };

        static constexpr bool Value = true;

        typedef TReturnType ReturnType;
        typedef void ClassType;

        typedef TReturnType(*FuncSignature)(Args...);

        typedef std::tuple<Args...> Tuple;

        template <SizeT i>
        struct Arg
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type Type;
            // the i-th argument is equivalent to the i-th tuple element of a tuple
            // composed of those arguments.
        };
    };

    template<class TCastClassType, typename T, typename = void>
    struct TMemberFunctionCast : TFFalseType
    {

    };

    template<class TCastClassType, typename TRetType, class TClassType, class... TArgs>
    struct TMemberFunctionCast<TCastClassType, TRetType(TClassType::*)(TArgs...)>
    {
        using TCastedFuncTraits = TFunctionTraits<TRetType(TCastClassType::*)(TArgs...)>;
        using TCastedFuncSignature = TCastedFuncTraits::FuncSignature;

        static constexpr bool Value = TCastedFuncTraits::Value;
    };
    
} // namespace Fusion
