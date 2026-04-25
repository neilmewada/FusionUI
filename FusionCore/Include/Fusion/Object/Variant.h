#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    class FUSIONCORE_API FVariant final
    {
    public:

        FVariant() = default;
        ~FVariant() { Free(); }

        FVariant(const FVariant& other)          { CopyFrom(other); }
        FVariant(FVariant&& other) noexcept      { MoveFrom(std::move(other)); }

        FVariant& operator=(const FVariant& other)
        {
            if (this != &other) CopyFrom(other);
            return *this;
        }

        FVariant& operator=(FVariant&& other) noexcept
        {
            if (this != &other) MoveFrom(std::move(other));
            return *this;
        }

        explicit FVariant(bool value)           { SetInternalValue(value); }
        explicit FVariant(i64 value)            { SetInternalValue(value); }
        explicit FVariant(u64 value)            { SetInternalValue(value); }
        explicit FVariant(f32 value)            { SetInternalValue(value); }
        explicit FVariant(f64 value)            { SetInternalValue(value); }
        FVariant(const FString& value)          { SetInternalValue(value); }
        FVariant(FString&& value)               { SetInternalValue(std::move(value)); }
        FVariant(const FName& value)            { SetInternalValue(value); }
        FVariant(FName&& value)                 { SetInternalValue(std::move(value)); }
        FVariant(FVec2 value)                   { SetInternalValue(value); }
        FVariant(FVec4 value)                   { SetInternalValue(value); }
        FVariant(FVec2i value)                  { SetInternalValue(value); }
        FVariant(FVec4i value)                  { SetInternalValue(value); }
        FVariant(FColor value)                  { SetInternalValue(value); }
        FVariant(FRect value)                   { SetInternalValue(value); }
        FVariant(FAffineTransform value)        { SetInternalValue(value); }
        FVariant(WeakRef<FObject> value)        { SetInternalValue(std::move(value)); }
        FVariant(void* value)                   { SetInternalValue(value); }

        template<typename T>
        const T& Get() const
        {
            FUSION_ASSERT(m_TypeID == FUSION_TYPEID(T), "FVariant type mismatch in Get<T>()");
            return *GetPtr<T>();
        }

        template<typename T>
        T& Get()
        {
            FUSION_ASSERT(m_TypeID == FUSION_TYPEID(T), "FVariant type mismatch in Get<T>()");
            return *GetPtr<T>();
        }

        bool HasValue() const { return m_TypeID != 0; }
        FTypeID GetTypeID() const { return m_TypeID; }

    private:

        template<typename T>
        T* GetPtr()
        {
            if constexpr      (TFIsSameType<T, bool>::Value)                  return &Bool;
            else if constexpr (TFIsSameType<T, i64>::Value)                   return &SignedInt;
            else if constexpr (TFIsSameType<T, u64>::Value)                   return &UnsignedInt;
            else if constexpr (TFIsSameType<T, f32>::Value)                   return &Float;
            else if constexpr (TFIsSameType<T, f64>::Value)                   return &Double;
            else if constexpr (TFIsSameType<T, FString>::Value)               return &String;
            else if constexpr (TFIsSameType<T, FName>::Value)                 return &Name;
            else if constexpr (TFIsSameType<T, FVec2>::Value)                 return &Vec2;
            else if constexpr (TFIsSameType<T, FVec4>::Value)                 return &Vec4;
            else if constexpr (TFIsSameType<T, FVec2i>::Value)                return &Vec2i;
            else if constexpr (TFIsSameType<T, FVec4i>::Value)                return &Vec4i;
            else if constexpr (TFIsSameType<T, FColor>::Value)                return &Color;
            else if constexpr (TFIsSameType<T, FRect>::Value)                 return &Rect;
            else if constexpr (TFIsSameType<T, FAffineTransform>::Value)      return &AffineTransform;
            else if constexpr (TFIsSameType<T, WeakRef<FObject>>::Value)      return &ObjectRef;
            else if constexpr (TFIsSameType<T, void*>::Value)                 return &RawPtr;
            else static_assert(sizeof(T) == 0, "FVariant: unsupported type");
        }

        template<typename T>
        const T* GetPtr() const { return const_cast<FVariant*>(this)->GetPtr<T>(); }

        template<typename T>
        void SetInternalValue(T&& value)
        {
            using DT = std::decay_t<T>;

            if (HasValue())
                Free();

            if constexpr (TFIsSameType<DT, FString>::Value)
                new (&String) FString(std::forward<T>(value));
            else if constexpr (TFIsSameType<DT, FName>::Value)
                new (&Name) FName(std::forward<T>(value));
            else if constexpr (TFIsSameType<DT, WeakRef<FObject>>::Value)
                new (&ObjectRef) WeakRef<FObject>(std::forward<T>(value));
            else
                *GetPtr<DT>() = std::forward<T>(value);

            m_TypeID = FUSION_TYPEID(DT);
        }

        void CopyFrom(const FVariant& other);
        void MoveFrom(FVariant&& other);
        void Free();

        FTypeID m_TypeID = 0;
        union
        {
            bool Bool;
            i64 SignedInt;
            u64 UnsignedInt;
            f32 Float;
            f64 Double;
            FString String;
            FName Name;
            FVec2 Vec2;
            FVec4 Vec4;
            FVec2i Vec2i;
            FVec4i Vec4i;
            FColor Color;
            FRect Rect;
            FAffineTransform AffineTransform;
            WeakRef<FObject> ObjectRef;
            void* RawPtr;
        };
    };
}
