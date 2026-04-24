#pragma once

// Copyright (c) 2026 Neil Mewada
// SPDX-License-Identifier: MIT

#include "Fusion/Delegates/Delegate.h"
#include "Fusion/Containers/Array.h"

#define FUSION_SIGNAL_TYPE(SignalType, ...) using SignalType = ::Fusion::FSignal<void(__VA_ARGS__)>;

namespace Fusion
{

    struct FSignalHandle
    {
        u32 ID = u32(-1);

        bool IsValid() const { return ID != u32(-1); }

        bool operator==(const FSignalHandle&) const = default;
        bool operator!=(const FSignalHandle&) const = default;

        static FSignalHandle Invalid() { return {}; }
    };

    // ---------------------------------------------------------------------------

    template<typename TSignature>
    class FSignal;

    template<typename TReturn, typename... TArgs>
    class FSignal<TReturn(TArgs...)>
    {
    public:
        FSignal()  = default;
        ~FSignal() = default;

        // Non-copyable — copying a signal's subscriber list is almost never intentional
        FSignal(const FSignal&)            = delete;
        FSignal& operator=(const FSignal&) = delete;

        FSignal(FSignal&&)            = default;
        FSignal& operator=(FSignal&&) = default;

        // ----------------------------------------------------------------
        // Add
        // ----------------------------------------------------------------

        // Lambda or free function — also accepts no-arg lambdas that ignore the signal's parameters
        template<typename TCallable>
        FSignalHandle Add(TCallable&& callable)
        {
            FSignalHandle handle{ m_NextID++ };
            auto& binding = m_Bindings.Emplace(handle.ID, FDelegate<TReturn(TArgs...)>{});

            if constexpr (std::is_invocable_v<TCallable, TArgs...>)
            {
                binding.Delegate.Bind(std::forward<TCallable>(callable));
            }
            else
            {
                // Callable doesn't accept TArgs — wrap it so parameters are discarded
                binding.Delegate.Bind([c = std::forward<TCallable>(callable)](TArgs...) mutable -> TReturn {
                    return c();
                });
            }

            return handle;
        }

        // Non-const member function (raw pointer)
        template<typename TObject, typename TMethod>
        FSignalHandle Add(TObject* object, TMethod method)
        {
            FSignalHandle handle{ m_NextID++ };
            auto& binding = m_Bindings.Emplace(handle.ID, FDelegate<TReturn(TArgs...)>{});
            binding.Delegate.Bind(object, method);
            return handle;
        }

        // Const member function (raw pointer)
        template<typename TObject, typename TMethod>
        FSignalHandle Add(const TObject* object, TMethod method)
        {
            FSignalHandle handle{ m_NextID++ };
            auto& binding = m_Bindings.Emplace(handle.ID, FDelegate<TReturn(TArgs...)>{});
            binding.Delegate.Bind(object, method);
            return handle;
        }

        // Non-const member function (Ptr<T>)
        template<typename TObject, typename TMethod>
        FSignalHandle Add(Ref<TObject> object, TMethod method)
        {
            FSignalHandle handle{ m_NextID++ };
            auto& binding = m_Bindings.Emplace(handle.ID, FDelegate<TReturn(TArgs...)>{});
            binding.Delegate.Bind(object, method);
            return handle;
        }

        // ----------------------------------------------------------------
        // Remove
        // ----------------------------------------------------------------

        void Remove(FSignalHandle handle)
        {
            for (SizeT i = 0; i < m_Bindings.Size(); ++i)
            {
                if (m_Bindings[i].ID == handle.ID)
                {
                    if (m_Broadcasting)
                    {
                        // Defer removal — Broadcast() will sweep after the loop
                        m_Bindings[i].Delegate.Unbind();
                    }
                    else
                    {
                        m_Bindings.RemoveAt(i);
                    }
                    return;
                }
            }
        }

        void RemoveAll()
        {
            if (m_Broadcasting)
            {
                for (SizeT i = 0; i < m_Bindings.Size(); ++i)
                    m_Bindings[i].Delegate.Unbind();
            }
            else
            {
                m_Bindings.Clear();
            }
        }

        // ----------------------------------------------------------------
        // Broadcast
        // ----------------------------------------------------------------

        void Broadcast(TArgs... args)
        {
            m_Broadcasting = true;

            for (SizeT i = 0; i < m_Bindings.Size(); ++i)
            {
                if (m_Bindings[i].Delegate.IsBound())
                    m_Bindings[i].Delegate.Execute(std::forward<TArgs>(args)...);
            }

            m_Broadcasting = false;

            // Sweep any entries that were unbound during broadcast
            for (SizeT i = m_Bindings.Size(); i > 0; --i)
            {
                if (!m_Bindings[i - 1].Delegate.IsBound())
                    m_Bindings.RemoveAtSwapLast(i - 1);
            }
        }

        // ----------------------------------------------------------------
        // Query
        // ----------------------------------------------------------------

        bool IsBound()  const { return !m_Bindings.Empty(); }
        SizeT Count()   const { return m_Bindings.Size(); }

    private:
        struct FBinding
        {
            u32 ID = u32(-1);
            FDelegate<TReturn(TArgs...)> Delegate;
        };

        TArray<FBinding, 1> m_Bindings;
        u32                 m_NextID      = 0;
        bool                m_Broadcasting = false;
    };

} // namespace Fusion
