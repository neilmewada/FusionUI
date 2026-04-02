#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "Fusion/Object/RefCountBlock.h"

#include <atomic>

#include "Ptr.h"
#include "WeakPtr.h"

#define FUSION_CLASS_BODY(SelfClass) typedef SelfClass Self;\
    virtual FTypeID GetClassTypeID() const { return ::Fusion::GetTypeID<Self>(); }\
    virtual FName GetClassName() const { thread_local Fusion::FName className = #SelfClass; return className; }

#define FUSION_CLASS(SelfClass, SuperClass) typedef SuperClass Super; \
    FUSION_CLASS_BODY(SelfClass)

namespace Fusion
{

	enum class EObjectFlags
    {
		None = 0,
        PendingConstruction = FUSION_BIT(0),
    };
    FUSION_ENUM_CLASS_FLAGS(EObjectFlags);

    class FUSIONCORE_API FObject
    {
		FUSION_CLASS_BODY(FObject)
    protected:
        FObject(FName name = "Object", Ptr<FObject> outer = nullptr);

    public:

        FObject(const FObject&)            = delete;
        FObject& operator=(const FObject&) = delete;

		void AttachSubobject(Ptr<FObject> subobject);
		void DetachSubobject(Ptr<FObject> subobject);

		Ptr<FObject> GetOuter() const { return m_Outer.Lock(); }

		u32 GetSubobjectCount() const { return static_cast<u32>(m_Subobjects.Size()); }

        Ptr<FObject> GetSubobject(u32 index) const
        {
            FUSION_ASSERT_THROW(index < static_cast<int>(m_Subobjects.Size()), FOutOfBoundsException, "Index out of bounds");
            return m_Subobjects[static_cast<size_t>(index)];
		}

		const FName& GetName() const { return m_Name; }

		void SetName(const FName& name) { m_Name = name; }

		FUuid GetUuid() const { return m_Uuid; }

    protected:

        virtual ~FObject();

        virtual void OnConstruct() {}

		virtual void OnSubobjectAttached(Ptr<FObject> subobject) {}

		virtual void OnSubobjectDetached(Ptr<FObject> subobject) {}

		virtual void OnDetachFromOuter() {}

		virtual void OnAttachToOuter() {}

        virtual void OnBeforeDestroy() {}

    private:

		FArray<Ptr<FObject>> m_Subobjects;

        FName m_Name;
		WeakPtr<FObject> m_Outer;
		FUuid m_Uuid;
        std::atomic<Internal::RefCountBlock*> m_Control = nullptr;
        EObjectFlags m_Flags = EObjectFlags::None;

        template<typename T> friend class Ptr;
        template<typename T> friend class WeakPtr;
        friend struct Internal::RefCountBlock;

        template<FObjectType TObject, typename... TArgs>
        friend TObject* NewObject(TArgs&&... args);
    };

    template<FObjectType TObject, typename... TArgs>
    TObject* NewObject(TArgs&&... args)
    {
        TObject* object = new TObject(std::forward<TArgs>(args)...);
        static_cast<FObject*>(object)->m_Flags &= ~EObjectFlags::PendingConstruction;
        static_cast<FObject*>(object)->OnConstruct();
        return object;
    }

} // namespace Fusion
