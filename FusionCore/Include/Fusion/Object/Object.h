#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "Fusion/Object/RefCountBlock.h"

#include <atomic>

#include "Ref.h"
#include "WeakRef.h"

#define FUSION_CLASS_BODY(SelfClass) typedef SelfClass Self;\
    public:\
		static FName StaticClassName() { thread_local const Fusion::FName className = #SelfClass; return className; }\
	    virtual FTypeID GetClassTypeID() const { thread_local const FTypeID typeId = ::Fusion::GetTypeID<Self>(); return typeId; }\
	    virtual FName GetClassName() const { thread_local const Fusion::FName className = #SelfClass; return className; }

#define FUSION_CLASS(SelfClass, SuperClass) typedef SuperClass Super; \
    FUSION_CLASS_BODY(SelfClass)\
    template<FObjectType TObject, typename... TArgs>\
	friend Ref<TObject> NewObject(FObject* outer, TArgs&&... args);

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

		FArray<Ref<FObject>> m_Subobjects;

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
        static_cast<FObject*>(object.Get())->m_Flags &= ~EObjectFlags::PendingConstruction;
        static_cast<FObject*>(object.Get())->OnConstruct();
        if (outer)
        {
            outer->AttachSubobject(object);
        }
        return object;
    }

} // namespace Fusion
