#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include <functional>

namespace Fusion
{

    template<typename TSignature>
    class FDelegate;

    template<typename TReturn, typename... TArgs>
    class FDelegate<TReturn(TArgs...)>
    {
    public:
        using FunctionType = std::function<TReturn(TArgs...)>;

        FDelegate() = default;

        FDelegate(std::nullptr_t) {}

        template<typename TCallable>
        FDelegate(TCallable&& callable)
            : m_Function(std::forward<TCallable>(callable))
        {
        }

        FDelegate(const FDelegate&)            = default;
        FDelegate(FDelegate&&)                 = default;
        FDelegate& operator=(const FDelegate&) = default;
        FDelegate& operator=(FDelegate&&)      = default;

        template<typename TCallable>
        FDelegate& operator=(TCallable&& callable)
        {
            m_Function = std::forward<TCallable>(callable);
            m_Target = nullptr;
            m_bHasTarget = false;
            return *this;
        }

        FDelegate& operator=(std::nullptr_t)
        {
            m_Function = nullptr;
            m_Target = nullptr;
            m_bHasTarget = false;
            return *this;
        }

        // Bind a lambda or free function
        template<typename TCallable>
        void Bind(TCallable&& callable)
        {
            m_Function = std::forward<TCallable>(callable);
            m_Target = nullptr;
            m_bHasTarget = false;
        }

        // Bind a non-const member function
        template<TObjectType TObject, typename TMethod>
        void Bind(TObject* object, TMethod method)
        {
            Ref<TObject> ref = object;
            if (ref.IsNull())
                return;

            m_Target = ref;
            m_bHasTarget = true;

            m_Function = [this, method](TArgs... args) -> TReturn
            {
                Ref<FObject> target = m_Target.Lock();
                FUSION_ASSERT(target.IsValid(), "Delegate called on a destroyed object!");

                Ref<TObject> casted = FObject::CastTo<TObject>(target);

                return (casted.Get()->*method)(std::forward<TArgs>(args)...);
            };
        }

        // Bind a non-const member function via Ref<T>
        template<TObjectType TObject, typename TMethod>
        void Bind(Ref<TObject> object, TMethod method)
        {
            if (object.IsNull())
                return;

            m_Target = object;
            m_bHasTarget = true;

            m_Function = [this, method](TArgs... args) -> TReturn
            {
                Ref<FObject> target = m_Target.Lock();
                FUSION_ASSERT(target.IsValid(), "Delegate called on a destroyed object!");

                Ref<TObject> casted = FObject::CastTo<TObject>(target);

                return (casted.Get()->*method)(std::forward<TArgs>(args)...);
            };
        }

        // Bind a non-const member function via Ref<T>
        template<TObjectType TObject, typename TMethod>
        void Bind(WeakRef<TObject> object, TMethod method)
        {
            Ref<FObject> ref = object.Lock();

            if (ref.IsNull())
                return;

            m_Target = object;
            m_bHasTarget = true;

            m_Function = [this, method](TArgs... args) -> TReturn
                {
                    Ref<FObject> target = m_Target.Lock();
                    FUSION_ASSERT(target.IsValid(), "Delegate called on a destroyed object!");

                    Ref<TObject> casted = FObject::CastTo<TObject>(target);

                    return (casted.Get()->*method)(std::forward<TArgs>(args)...);
                };
        }

        bool IsBound() const { return static_cast<bool>(m_Function) && (!m_bHasTarget || m_Target.IsValid()); }

        explicit operator bool() const { return IsBound(); }

        TReturn operator()(TArgs... args) const
        {
            return m_Function(std::forward<TArgs>(args)...);
        }

        TReturn Execute(TArgs... args) const
        {
            return m_Function(std::forward<TArgs>(args)...);
        }

        // Calls the delegate only if it is bound. Returns true if it was called.
        // Only usable when TReturn is void.
        [[maybe_unused]] bool ExecuteIfBound(TArgs... args) const requires std::is_void_v<TReturn>
        {
            if (!IsBound())
                return false;
            m_Function(std::forward<TArgs>(args)...);
            return true;
        }

        void Unbind() { m_Function = nullptr; }

        const FunctionType& GetFunction() const { return m_Function; }

        Ref<FObject> GetTarget() const { return m_Target.Lock(); }

    private:
        FunctionType m_Function;
        WeakRef<FObject> m_Target;
        bool m_bHasTarget = false;
    };

} // namespace Fusion
