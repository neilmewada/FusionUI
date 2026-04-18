#pragma once

#include <string>
#include <string_view>
#include <format>
#include <cstdint>

namespace Fusion
{

    // UTF-8 string with small string optimization (SSO up to 15 bytes inline).
    // ByteLength() — number of bytes.
    // Codepoints() — forward range yielding char32_t Unicode codepoints.
    class FUSIONCORE_API FString
    {
    public:
        struct CodepointIterator
        {
            using value_type      = char32_t;
            using difference_type = std::ptrdiff_t;

            const char* Ptr;

            char32_t operator*() const
            {
                auto b = static_cast<uint8_t>(*Ptr);
                if (b < 0x80) return b;
                if (b < 0xE0) return ((b & 0x1F) << 6)  |  (static_cast<uint8_t>(Ptr[1]) & 0x3F);
                if (b < 0xF0) return ((b & 0x0F) << 12) | ((static_cast<uint8_t>(Ptr[1]) & 0x3F) << 6)  |  (static_cast<uint8_t>(Ptr[2]) & 0x3F);
                              return ((b & 0x07) << 18) | ((static_cast<uint8_t>(Ptr[1]) & 0x3F) << 12) | ((static_cast<uint8_t>(Ptr[2]) & 0x3F) << 6) | (static_cast<uint8_t>(Ptr[3]) & 0x3F);
            }

            CodepointIterator& operator++()
            {
                auto b = static_cast<uint8_t>(*Ptr);
                if      (b < 0x80) Ptr += 1;
                else if (b < 0xE0) Ptr += 2;
                else if (b < 0xF0) Ptr += 3;
                else               Ptr += 4;
                return *this;
            }

            CodepointIterator operator++(int)
            {
                CodepointIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const CodepointIterator& o) const { return Ptr == o.Ptr; }
            bool operator!=(const CodepointIterator& o) const { return Ptr != o.Ptr; }
        };

        struct CodepointRange
        {
            const char* Begin;
            const char* End;

            CodepointIterator begin() const { return { Begin }; }
            CodepointIterator end()   const { return { End };   }
        };

        FString();
        FString(const char* str);
        FString(const char* str, size_t length);
        FString(std::string_view str);
        FString(const std::string& str);
        FString(const FString& other);
        FString(FString&& other) noexcept;
        ~FString();

        FString& operator=(const FString& other);
        FString& operator=(FString&& other) noexcept;
        FString& operator=(const char* str);
        FString& operator=(std::string_view str);

        // Returns the raw UTF-8 byte buffer, null-terminated.
        const char*      CStr()        const;
        std::string_view View()        const;
        std::string      ToStdString() const;

        size_t             Size()   const { return m_Size; }
        size_t             Length() const { return m_Size; }
        size_t         ByteLength() const { return m_Size; }
        CodepointRange Codepoints() const { return { Data(), Data() + m_Size }; }
        bool                Empty() const { return m_Size == 0; }

        FString& operator+=(const FString& other);
        FString& operator+=(const std::string& str);
        FString& operator+=(std::string_view str);
        FString& operator+=(const char* str);

        FString operator+(const FString& other)      const;
        FString operator+(const std::string& other)  const;
        FString operator+(std::string_view other)    const;
        FString operator+(const char* other)         const;

        bool operator==(const FString& other)      const;
        bool operator==(const std::string& other)  const;
        bool operator==(std::string_view other)    const;
        bool operator==(const char* other)         const;

        bool operator!=(const FString& other)      const { return !(*this == other); }
        bool operator!=(const std::string& other)  const { return !(*this == other); }
        bool operator!=(std::string_view other)    const { return !(*this == other); }
        bool operator!=(const char* other)         const { return !(*this == other); }

        SizeT GetHash() const;

        // ----------------------------------------------------------------
        // Formatting
        // ----------------------------------------------------------------

        // Compile-time checked format string. Usage: FString::Format("Hello {}!", name)
        template<typename... Args>
        static FString Format(std::format_string<Args...> fmt, Args&&... args)
        {
            return FString(std::format(fmt, std::forward<Args>(args)...));
        }

        // Runtime format string. Usage: FString::FormatV(runtimeFmt, std::make_format_args(a, b))
        static FString FormatV(std::string_view fmt, std::format_args args)
        {
            return FString(std::vformat(fmt, args));
        }

    private:
        static constexpr size_t SSO_CAPACITY = 15;

        struct HeapBuffer
        {
            char*  Ptr;
            size_t Capacity;
        };

        union
        {
            HeapBuffer m_Heap;
            char       m_SSO[SSO_CAPACITY + 1];
        };

        size_t m_Size = 0;

        bool        IsHeap()       const { return m_Size > SSO_CAPACITY; }
        char*       Data()               { return IsHeap() ? m_Heap.Ptr : m_SSO; }
        const char* Data()         const { return IsHeap() ? m_Heap.Ptr : m_SSO; }

        void Assign(const char* str, size_t length);
        void AppendBytes(const char* str, size_t length);
    };

    template <typename ... Args>
    void LogFormat(FLogLevel level, const char* category, const char* message, Args&&... args)
    {
		Fusion::Log(level, category, FString::FormatV(message, std::make_format_args(args...)));
    }

    template <SizeT Bits>
    FString FBitSet<Bits>::ToString() const
    {
        return impl.to_string();
    }

} // namespace Fusion

// Allow FString to be used directly as a std::format argument.
// Example: FString::Format("Value: {}", someString)
template<>
struct std::formatter<Fusion::FString> : std::formatter<std::string_view>
{
    auto format(const Fusion::FString& str, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(str.View(), ctx);
    }
};

