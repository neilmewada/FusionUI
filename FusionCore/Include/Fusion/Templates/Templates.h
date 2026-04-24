#pragma once

#include <bitset>
#include <memory>

namespace Fusion
{
    class FString;

    template<typename T>
    struct TNumericLimits final
    {
        TNumericLimits() = delete;

        constexpr static T Min() noexcept
        {
            return std::numeric_limits<T>::min();
        }

        constexpr static T Max() noexcept
        {
            return std::numeric_limits<T>::max();
        }

        constexpr static T Infinity() noexcept
        {
            return std::numeric_limits<T>::infinity();
        }
    };

    template<SizeT Bits = 32>
    class TBitSet
    {
    public:

        TBitSet(SizeT value = 0) : impl(value)
        {}

        inline void Set(SizeT pos, bool value = true)
        {
            impl.set(pos, value);
        }

        inline void Set()
        {
            impl.set();
        }

        /// @brief Flips all bits
        inline void Flip()
        {
            impl.flip();
        }

        inline void Reset()
        {
            impl.reset();
        }

        inline bool Test(SizeT pos) const
        {
            return impl.test(pos);
        }

        inline SizeT Count() const
        {
            return impl.count();
        }

        inline bool Any() const
        {
            return impl.any();
        }

        inline bool All() const
        {
            return impl.all();
        }

        inline bool None() const
        {
            return impl.none();
        }

        inline u32 ToU32() const
        {
            return impl.to_ulong();
        }

        inline u64 ToU64() const
        {
            return impl.to_ullong();
        }

        FString ToString() const;

        constexpr SizeT GetSize() const { return impl.size(); }

        inline bool operator==(const TBitSet& rhs) const
        {
            return impl == rhs.impl;
        }

        inline bool operator!=(const TBitSet& rhs) const
        {
            return impl != rhs.impl;
        }

        TBitSet& operator|=(const TBitSet& rhs)
        {
            impl |= rhs.impl;
            return *this;
        }

        TBitSet& operator&=(const TBitSet& rhs)
        {
            impl &= rhs.impl;
            return *this;
        }

        TBitSet& operator^=(const TBitSet& rhs)
        {
            impl ^= rhs.impl;
            return *this;
        }

        TBitSet& operator<<=(const TBitSet& rhs)
        {
            impl <<= rhs.impl;
            return *this;
        }

        TBitSet& operator>>=(const TBitSet& rhs)
        {
            impl >>= rhs.impl;
            return *this;
        }

    private:

        std::bitset<Bits> impl{};
    };

    template <auto Start, auto End, auto Inc, class F>
    constexpr void constexpr_for(F&& f)
    {
        if constexpr (Start < End)
        {
            f(std::integral_constant<decltype(Start), Start>());
            constexpr_for<Start + Inc, End, Inc>(f);
        }
    }

    template<typename T>
    class TUniquePtr
    {
    public:

        TUniquePtr(T* ptr = nullptr) : impl(ptr)
        {

        }

        TUniquePtr(std::unique_ptr<T>&& move) : impl(std::move(move))
        {

        }

        inline void Reset(T* ptr)
        {
            impl.reset(ptr);
        }

        inline T* Release()
        {
            return impl.release();
        }

        inline T* Get() const
        {
            return impl.get();
        }

        inline operator T* () const
        {
            return impl.get();
        }

        inline T* operator->() const
        {
            return impl.get();
        }

    private:
        std::unique_ptr<T> impl = nullptr;
    };

    template<typename T, typename... TArgs>
    TUniquePtr<T> MakeUnique(TArgs&&... args)
    {
        return TUniquePtr<T>(new T(args...));
    }

} // namespace Fusion
