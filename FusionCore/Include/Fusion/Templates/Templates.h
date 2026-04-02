#pragma once

#include <bitset>

namespace Fusion
{
    class FString;

    template<typename T>
    struct FNumericLimits final
    {
        FNumericLimits() = delete;

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
    class FBitSet
    {
    public:

        FBitSet(SizeT value = 0) : impl(value)
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

        inline bool operator==(const FBitSet& rhs) const
        {
            return impl == rhs.impl;
        }

        inline bool operator!=(const FBitSet& rhs) const
        {
            return impl != rhs.impl;
        }

        FBitSet& operator|=(const FBitSet& rhs)
        {
            impl |= rhs.impl;
            return *this;
        }

        FBitSet& operator&=(const FBitSet& rhs)
        {
            impl &= rhs.impl;
            return *this;
        }

        FBitSet& operator^=(const FBitSet& rhs)
        {
            impl ^= rhs.impl;
            return *this;
        }

        FBitSet& operator<<=(const FBitSet& rhs)
        {
            impl <<= rhs.impl;
            return *this;
        }

        FBitSet& operator>>=(const FBitSet& rhs)
        {
            impl >>= rhs.impl;
            return *this;
        }

    private:

        std::bitset<Bits> impl{};
    };

} // namespace Fusion
