#pragma once

#define FUSION_DEFINE_HANDLE_TYPE(HandleTypeName, ...)\
    class HandleTypeName\
    {\
    public:\
		using IndexType = THandle<__VA_ARGS__>::IndexType;\
		static constexpr THandle<__VA_ARGS__> NullValue = THandle<__VA_ARGS__>::NullValue;\
        HandleTypeName() = default;\
        explicit HandleTypeName(THandle<__VA_ARGS__> value) : m_Value(value) {}\
        bool IsNull() const { return m_Value.IsNull(); }\
        bool IsValid() const { return m_Value.IsNull(); }\
        SizeT GetHash() const { return m_Value.GetHash(); }\
        auto Get() const { return m_Value.Get(); }\
        bool operator==(const HandleTypeName& rhs) const { return m_Value == rhs.m_Value; }\
        bool operator!=(const HandleTypeName& rhs) const { return m_Value != rhs.m_Value; }\
    private:\
        THandle<__VA_ARGS__> m_Value = THandle<__VA_ARGS__>::NullValue;\
    };

namespace Fusion
{

    template<typename T = u32> requires TFIsIntegralType<T>::Value
    class THandle
    {
    public:
        using IndexType = T;

        static constexpr T NullValue = T(-1);

        constexpr THandle() : value(NullValue)
        {

        }

        constexpr THandle(T value) : value(value)
        {

        }

        inline operator T() const
        {
            return value;
        }

        inline bool IsNull() const
        {
            return value == NullValue;
        }

        inline bool IsValid() const
        {
            return !IsNull();
        }

        inline T Get() const
        {
            return value;
        }

        inline void Reset()
        {
            value = NullValue;
        }

        SizeT GetHash() const
        {
            return ::Fusion::GetHash<T>(value);
        }

    private:
        T value = NullValue;
    };

} // namespace Fusion
