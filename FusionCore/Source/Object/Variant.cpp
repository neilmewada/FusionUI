#include "Fusion/Core.h"

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

namespace Fusion
{
    void FVariant::CopyFrom(const FVariant& other)
    {
        if (HasValue())
            Free();

        if (!other.HasValue())
            return;

        if (other.m_TypeID == FUSION_TYPEID(FString))
            new (&String) FString(other.String);
        else if (other.m_TypeID == FUSION_TYPEID(FName))
            new (&Name) FName(other.Name);
        else if (other.m_TypeID == FUSION_TYPEID(WeakRef<FObject>))
            new (&ObjectRef) WeakRef<FObject>(other.ObjectRef);
        else if (other.m_TypeID == FUSION_TYPEID(bool))
            Bool = other.Bool;
        else if (other.m_TypeID == FUSION_TYPEID(i64))
            SignedInt = other.SignedInt;
        else if (other.m_TypeID == FUSION_TYPEID(u64))
            UnsignedInt = other.UnsignedInt;
        else if (other.m_TypeID == FUSION_TYPEID(f32))
            Float = other.Float;
        else if (other.m_TypeID == FUSION_TYPEID(f64))
            Double = other.Double;
        else if (other.m_TypeID == FUSION_TYPEID(FVec2))
            Vec2 = other.Vec2;
        else if (other.m_TypeID == FUSION_TYPEID(FVec4))
            Vec4 = other.Vec4;
        else if (other.m_TypeID == FUSION_TYPEID(FVec2i))
            Vec2i = other.Vec2i;
        else if (other.m_TypeID == FUSION_TYPEID(FVec4i))
            Vec4i = other.Vec4i;
        else if (other.m_TypeID == FUSION_TYPEID(FColor))
            Color = other.Color;
        else if (other.m_TypeID == FUSION_TYPEID(FRect))
            Rect = other.Rect;
        else if (other.m_TypeID == FUSION_TYPEID(FAffineTransform))
            AffineTransform = other.AffineTransform;
        else if (other.m_TypeID == FUSION_TYPEID(void*))
            RawPtr = other.RawPtr;

        m_TypeID = other.m_TypeID;
    }

    void FVariant::MoveFrom(FVariant&& other)
    {
        if (HasValue())
            Free();

        if (!other.HasValue())
            return;

        if (other.m_TypeID == FUSION_TYPEID(FString))
        {
            new (&String) FString(std::move(other.String));
            other.String.~FString();
        }
        else if (other.m_TypeID == FUSION_TYPEID(FName))
        {
            new (&Name) FName(std::move(other.Name));
            other.Name.~FName();
        }
        else if (other.m_TypeID == FUSION_TYPEID(WeakRef<FObject>))
        {
            new (&ObjectRef) WeakRef<FObject>(std::move(other.ObjectRef));
            other.ObjectRef.~WeakRef<FObject>();
        }
        else if (other.m_TypeID == FUSION_TYPEID(bool))
            Bool = other.Bool;
        else if (other.m_TypeID == FUSION_TYPEID(i64))
            SignedInt = other.SignedInt;
        else if (other.m_TypeID == FUSION_TYPEID(u64))
            UnsignedInt = other.UnsignedInt;
        else if (other.m_TypeID == FUSION_TYPEID(f32))
            Float = other.Float;
        else if (other.m_TypeID == FUSION_TYPEID(f64))
            Double = other.Double;
        else if (other.m_TypeID == FUSION_TYPEID(FVec2))
            Vec2 = other.Vec2;
        else if (other.m_TypeID == FUSION_TYPEID(FVec4))
            Vec4 = other.Vec4;
        else if (other.m_TypeID == FUSION_TYPEID(FVec2i))
            Vec2i = other.Vec2i;
        else if (other.m_TypeID == FUSION_TYPEID(FVec4i))
            Vec4i = other.Vec4i;
        else if (other.m_TypeID == FUSION_TYPEID(FColor))
            Color = other.Color;
        else if (other.m_TypeID == FUSION_TYPEID(FRect))
            Rect = other.Rect;
        else if (other.m_TypeID == FUSION_TYPEID(FAffineTransform))
            AffineTransform = other.AffineTransform;
        else if (other.m_TypeID == FUSION_TYPEID(void*))
            RawPtr = other.RawPtr;

        m_TypeID = other.m_TypeID;
        other.m_TypeID = 0;
    }

    void FVariant::Free()
    {
        if (!HasValue())
            return;

        if (m_TypeID == FUSION_TYPEID(FString))
            String.~FString();
        else if (m_TypeID == FUSION_TYPEID(FName))
            Name.~FName();
        else if (m_TypeID == FUSION_TYPEID(WeakRef<FObject>))
            ObjectRef.~WeakRef<FObject>();

        m_TypeID = 0;
    }
}
