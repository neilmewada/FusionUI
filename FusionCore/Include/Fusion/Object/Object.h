#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "Fusion/Object/RefCountBlock.h"

#include <atomic>
#include <type_traits>

#include "Ref.h"
#include "WeakRef.h"

#define FUSION_CLASS_BODY(SelfClass) typedef SelfClass Self;\
    public:\
		static FName StaticClassName() { thread_local const Fusion::FName className = #SelfClass; return className; }\
		static FTypeID StaticClassTypeID() { thread_local const FTypeID typeId = ::Fusion::GetTypeID<Self>(); return typeId; }\
	    virtual FTypeID GetClassTypeID() const { thread_local const FTypeID typeId = ::Fusion::GetTypeID<Self>(); return typeId; }\
	    virtual FName GetClassName() const { thread_local const Fusion::FName className = #SelfClass; return className; }

#define FUSION_CLASS(SelfClass, SuperClass) typedef SuperClass Super; \
    FUSION_CLASS_BODY(SelfClass)\
    static FTypeID StaticSuperClassTypeID() { thread_local const FTypeID typeId = ::Fusion::GetTypeID<Super>(); return typeId; }\
    template<FObjectType TObject, typename... TArgs>\
	friend Ref<TObject> NewObject(FObject* outer, TArgs&&... args);\
    bool IsOfType(FTypeID typeId) const override\
	{\
		static_assert(std::is_base_of_v<SuperClass, SelfClass>, "The base class mentioned in FUSION_WIDGET/FUSION_CLASS does not match with actual base class in " #SelfClass);\
		return typeId == ::Fusion::GetTypeID<SelfClass>() || Super::IsOfType(typeId);\
    }

namespace Fusion
{
    template<FObjectType TObject, typename... TArgs>
    Ref<TObject> NewObject(FObject* outer, TArgs&&... args);

	enum class EObjectFlags
    {
		None = 0,
        PendingConstruction = FUSION_BIT(0),
        PendingDestruction = FUSION_BIT(1),
    };
    FUSION_ENUM_CLASS_FLAGS(EObjectFlags);

    class FUSIONCORE_API FObject
    {
		FUSION_CLASS_BODY(FObject)
    protected:
        FObject(FName name = "Object");

        // Creates a subobject during this object's construction.
        // Only valid inside a constructor (guarded by PendingConstruction flag).
        // Avoids the WeakRef-to-uninitialized-control-block crash that NewObject
        // would cause when called with 'this' as outer during construction.
        template<FObjectType TObject, typename... TArgs>
        Ref<TObject> CreateSubobject(TArgs&&... args)
        {
            FUSION_ASSERT(FEnumHasFlag(m_Flags, EObjectFlags::PendingConstruction),
                "CreateSubobject can only be called during construction. Use NewObject after construction.");

            Ref<TObject> object = new TObject(std::forward<TArgs>(args)...);

            FObject* raw = static_cast<FObject*>(object.Get());
            raw->m_Flags &= ~EObjectFlags::PendingConstruction;

            // object has a valid control block now — fix up its own subobjects
            // (created via CreateSubobject inside object's constructor)
            for (auto& subobject : raw->m_Subobjects)
            {
                if (subobject->m_Outer.IsNull())
                    subobject->m_Outer = object;
            }

            raw->OnConstruct();

            // Push directly into subobjects — do NOT call AttachSubobject which
            // would create a WeakRef(this) while this->m_Control is still null.
            // m_Outer on object is left null here and fixed up by NewObject (or the
            // parent's CreateSubobject) after this object's construction completes.
            m_Subobjects.Add(object);

            return object;
        }

    public:

        FObject(const FObject&)            = delete;
        FObject& operator=(const FObject&) = delete;

		void AttachSubobject(Ref<FObject> subobject);
		void DetachSubobject(Ref<FObject> subobject);

		Ref<FObject> GetOuter() const { return m_Outer.Lock(); }

		u32 GetSubobjectCount() const { return static_cast<u32>(m_Subobjects.Size()); }

        EObjectFlags GetFlags() const { return m_Flags; }

        bool IsPendingDestruction() const { return FEnumHasFlag(m_Flags, EObjectFlags::PendingDestruction); }

        Ref<FObject> GetSubobject(u32 index) const
        {
            FUSION_ASSERT_THROW(index < static_cast<int>(m_Subobjects.Size()), FOutOfBoundsException, "Index out of bounds");
            return m_Subobjects[static_cast<size_t>(index)];
		}

		const FName& GetName() const { return m_Name; }

		void SetName(const FName& name) { m_Name = name; }

		FUuid GetUuid() const { return m_Uuid; }

        virtual bool IsOfType(FTypeID typeId) const
        {
            return typeId == ::Fusion::GetTypeID<FObject>();
        }

        template<typename TObject>
        bool IsOfStaticType() const
		{
            return IsOfType(TObject::StaticClassTypeID());
		}

        template<typename TTo> requires TFIsDerivedClass<FObject, TTo>::Value
        Ref<TTo> Cast()
		{
            if (IsOfStaticType<TTo>())
            {
                return (TTo*)this;
            }
            return nullptr;
		}

        template<typename TTo, typename TFrom> requires TFIsDerivedClass<FObject, TTo>::Value and TFIsDerivedClass<FObject, TFrom>::Value
        static Ref<TTo> CastTo(Ref<TFrom> object)
		{
			if (object && object->template IsOfStaticType<TTo>())
			{
                return (TTo*)object.Get();
			}

            return nullptr;
		}

        template<typename TTo, typename TFrom> requires TFIsDerivedClass<FObject, TTo>::Value and TFIsDerivedClass<FObject, TFrom>::Value
        static Ref<TTo> StaticCastTo(Ref<TFrom> object)
        {
            return (TTo*)object.Get();
        }

        void BeginDestroy();

    protected:

        virtual ~FObject();

        virtual void OnConstruct() {}

		virtual void OnSubobjectAttached(Ref<FObject> subobject) {}

		virtual void OnSubobjectDetached(Ref<FObject> subobject) {}

		virtual void OnDetachFromOuter() {}

		virtual void OnAttachToOuter() {}

        virtual void OnBeforeDestroy() {}

    private:

		TArray<Ref<FObject>, 4> m_Subobjects;

        FName m_Name;
		WeakRef<FObject> m_Outer;
		FUuid m_Uuid;
        std::atomic<Internal::RefCountBlock*> m_Control = nullptr;
        EObjectFlags m_Flags = EObjectFlags::None;

        template<typename T> friend class Ref;
        template<typename T> friend class WeakRef;
        friend struct Internal::RefCountBlock;

        template<FObjectType TObject, typename... TArgs>
        friend Ref<TObject> NewObject(FObject* outer, TArgs&&... args);
    };

    template<FObjectType TObject, typename... TArgs>
    Ref<TObject> NewObject(FObject* outer, TArgs&&... args)
    {
        Ref<TObject> object = new TObject(std::forward<TArgs>(args)...);

        FObject* raw = static_cast<FObject*>(object.Get());
        raw->m_Flags &= ~EObjectFlags::PendingConstruction;

        // Fix up outers for any subobjects created via CreateSubobject during
        // construction. Control block is valid now so WeakRef assignment is safe.
        for (auto& subobject : raw->m_Subobjects)
        {
            if (subobject->m_Outer.IsNull())
                subobject->m_Outer = object;
        }

        raw->OnConstruct();

        if (outer)
        {
            outer->AttachSubobject(object);
        }
        return object;
    }

    template<class TObject>
    concept TObjectType = std::is_base_of_v<FObject, TObject>;

} // namespace Fusion
