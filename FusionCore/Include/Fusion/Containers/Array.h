#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include <initializer_list>

#include "Fusion/Misc/Assert.h"

namespace Fusion
{
    template<typename T, size_t InlineCapacity = 8>
    class FArray
    {
    public:
        static constexpr size_t npos = static_cast<size_t>(-1);

        // ----------------------------------------------------------------
        // Construction / Destruction
        // ----------------------------------------------------------------

        FArray()
        {
            InitInline();
        }

        FArray(std::initializer_list<T> list)
        {
            InitInline();
            Reserve(list.size());
            for (const T& value : list)
                Add(value);
        }

        FArray(const FArray& other)
        {
            InitInline();
            Reserve(other.m_Size);
            for (size_t i = 0; i < other.m_Size; ++i)
                new (m_Data + i) T(other.m_Data[i]);
            m_Size = other.m_Size;
        }

        FArray(FArray&& other) noexcept
        {
            InitInline();
            MoveFrom(std::move(other));
        }

        ~FArray()
        {
            DestroyElements(0, m_Size);
            FreeHeap();
        }

        // ----------------------------------------------------------------
        // Assignment
        // ----------------------------------------------------------------

        FArray& operator=(const FArray& other)
        {
            if (this != &other)
            {
                Clear();
                Reserve(other.m_Size);
                for (size_t i = 0; i < other.m_Size; ++i)
                    new (m_Data + i) T(other.m_Data[i]);
                m_Size = other.m_Size;
            }
            return *this;
        }

        FArray& operator=(FArray&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                FreeHeap();
                InitInline();
                MoveFrom(std::move(other));
            }
            return *this;
        }

        // ----------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------

        T&       operator[](size_t index)       { FUSION_ASSERT(index < m_Size, "FArray: index out of bounds"); return m_Data[index]; }
        const T& operator[](size_t index) const { FUSION_ASSERT(index < m_Size, "FArray: index out of bounds"); return m_Data[index]; }

        T&       First()       { FUSION_ASSERT(m_Size > 0, "FArray: First() called on empty array"); return m_Data[0]; }
        const T& First() const { FUSION_ASSERT(m_Size > 0, "FArray: First() called on empty array"); return m_Data[0]; }

        T&       Last()        { FUSION_ASSERT(m_Size > 0, "FArray: Last() called on empty array"); return m_Data[m_Size - 1]; }
        const T& Last()  const { FUSION_ASSERT(m_Size > 0, "FArray: Last() called on empty array"); return m_Data[m_Size - 1]; }

        T*       Data()        { return m_Data; }
        const T* Data()  const { return m_Data; }

        // ----------------------------------------------------------------
        // Capacity
        // ----------------------------------------------------------------

        size_t Size()     const { return m_Size; }
        size_t Capacity() const { return m_Capacity; }
        bool   IsEmpty()  const { return m_Size == 0; }

        void Reserve(size_t newCapacity)
        {
            if (newCapacity > m_Capacity)
                Grow(newCapacity);
        }

        void Resize(size_t newSize)
        {
            if (newSize > m_Size)
            {
                Reserve(newSize);
                for (size_t i = m_Size; i < newSize; ++i)
                    new (m_Data + i) T();
            }
            else
            {
                DestroyElements(newSize, m_Size);
            }
            m_Size = newSize;
        }

        void Resize(size_t newSize, const T& defaultValue)
        {
            if (newSize > m_Size)
            {
                Reserve(newSize);
                for (size_t i = m_Size; i < newSize; ++i)
                    new (m_Data + i) T(defaultValue);
            }
            else
            {
                DestroyElements(newSize, m_Size);
            }
            m_Size = newSize;
        }

        // ----------------------------------------------------------------
        // Modifiers
        // ----------------------------------------------------------------

        void Add(const T& value)
        {
            EnsureCapacity();
            new (m_Data + m_Size) T(value);
            ++m_Size;
        }

        void Add(T&& value)
        {
            EnsureCapacity();
            new (m_Data + m_Size) T(std::move(value));
            ++m_Size;
        }

        template<typename... Args>
        T& Emplace(Args&&... args)
        {
            EnsureCapacity();
            T* ptr = new (m_Data + m_Size) T(std::forward<Args>(args)...);
            ++m_Size;
            return *ptr;
        }

        void Pop()
        {
            FUSION_ASSERT(m_Size > 0, "FArray: Pop() called on empty array");
            m_Data[m_Size - 1].~T();
            --m_Size;
        }

        // Ordered remove — preserves element order, O(n)
        void RemoveAt(size_t index)
        {
            FUSION_ASSERT(index < m_Size, "FArray: RemoveAt() index out of bounds");
            m_Data[index].~T();
            for (size_t i = index; i < m_Size - 1; ++i)
            {
                new (m_Data + i) T(std::move(m_Data[i + 1]));
                m_Data[i + 1].~T();
            }
            --m_Size;
        }

        // Unordered remove — swaps with last element, O(1)
        void RemoveAtSwap(size_t index)
        {
            FUSION_ASSERT(index < m_Size, "FArray: RemoveAtSwap() index out of bounds");
            m_Data[index].~T();
            if (index != m_Size - 1)
            {
                new (m_Data + index) T(std::move(m_Data[m_Size - 1]));
                m_Data[m_Size - 1].~T();
            }
            --m_Size;
        }

        void Clear()
        {
            DestroyElements(0, m_Size);
            m_Size = 0;
        }

        // ----------------------------------------------------------------
        // Search
        // ----------------------------------------------------------------

        bool Contains(const T& value) const
        {
            for (size_t i = 0; i < m_Size; ++i)
                if (m_Data[i] == value)
                    return true;
            return false;
        }

        size_t IndexOf(const T& value) const
        {
            for (size_t i = 0; i < m_Size; ++i)
                if (m_Data[i] == value)
                    return i;
            return npos;
        }

        // ----------------------------------------------------------------
        // Iterators
        // ----------------------------------------------------------------

        T*       begin()       { return m_Data; }
        T*       end()         { return m_Data + m_Size; }
        const T* begin() const { return m_Data; }
        const T* end()   const { return m_Data + m_Size; }

    private:
        // Use at least 1 byte so zero InlineCapacity doesn't produce a zero-length array
        static constexpr size_t StorageSize = InlineCapacity > 0 ? InlineCapacity : 1;

        alignas(T) uint8_t m_InlineBuffer[sizeof(T) * StorageSize];
        T*     m_Data     = nullptr;
        size_t m_Size     = 0;
        size_t m_Capacity = 0;

        // ----------------------------------------------------------------
        // Helpers
        // ----------------------------------------------------------------

        T* InlineData()
        {
            return std::launder(reinterpret_cast<T*>(m_InlineBuffer));
        }

        const T* InlineData() const
        {
            return std::launder(reinterpret_cast<const T*>(m_InlineBuffer));
        }

        bool IsInline() const
        {
            return m_Data == std::launder(reinterpret_cast<const T*>(m_InlineBuffer));
        }

        void InitInline()
        {
            if constexpr (InlineCapacity > 0)
            {
                m_Data     = InlineData();
                m_Capacity = InlineCapacity;
            }
            else
            {
                m_Data     = nullptr;
                m_Capacity = 0;
            }
            m_Size = 0;
        }

        void EnsureCapacity()
        {
            if (m_Size == m_Capacity)
                Grow(m_Capacity == 0 ? 1 : m_Capacity * 2);
        }

        static T* Allocate(size_t count)
        {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
                return static_cast<T*>(::operator new(sizeof(T) * count, std::align_val_t{ alignof(T) }));
            else
                return static_cast<T*>(::operator new(sizeof(T) * count));
        }

        static void Deallocate(T* ptr)
        {
            if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
                ::operator delete(ptr, std::align_val_t{ alignof(T) });
            else
                ::operator delete(ptr);
        }

        void Grow(size_t newCapacity)
        {
            T* newData = Allocate(newCapacity);

            for (size_t i = 0; i < m_Size; ++i)
            {
                new (newData + i) T(std::move(m_Data[i]));
                m_Data[i].~T();
            }

            FreeHeap();

            m_Data     = newData;
            m_Capacity = newCapacity;
        }

        void FreeHeap()
        {
            if (!IsInline() && m_Data != nullptr)
                Deallocate(m_Data);
        }

        void DestroyElements(size_t from, size_t to)
        {
            for (size_t i = from; i < to; ++i)
                m_Data[i].~T();
        }

        void MoveFrom(FArray&& other)
        {
            if (other.IsInline())
            {
                // Can't steal inline buffer — move elements individually
                if constexpr (InlineCapacity > 0)
                {
                    for (size_t i = 0; i < other.m_Size; ++i)
                    {
                        new (m_Data + i) T(std::move(other.m_Data[i]));
                        other.m_Data[i].~T();
                    }
                }
                m_Size         = other.m_Size;
                other.m_Size   = 0;
            }
            else
            {
                // Steal the heap buffer
                m_Data         = other.m_Data;
                m_Size         = other.m_Size;
                m_Capacity     = other.m_Capacity;

                // Reset other to empty inline state
                other.InitInline();
            }
        }
    };

} // namespace Fusion
