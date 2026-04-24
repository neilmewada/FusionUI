#include "Fusion/Containers/String.h"

#include <cstring>
#include <algorithm>

namespace Fusion
{

    FString::FString()
    {
        m_SSO[0] = '\0';
        m_Size   = 0;
    }

    FString::FString(const char* str)
    {
        if (str)
            Assign(str, std::strlen(str));
        else
        {
            m_SSO[0] = '\0';
            m_Size   = 0;
        }
    }

    FString::FString(const char* str, size_t length)
    {
        Assign(str, length);
    }

    FString::FString(std::string_view str)
    {
        Assign(str.data(), str.size());
    }

    FString::FString(const std::string& str)
    {
        Assign(str.data(), str.size());
    }

    FString::FString(const FString& other)
    {
        Assign(other.Data(), other.m_Size);
    }

    FString::FString(FString&& other) noexcept
    {
        m_Size = other.m_Size;

        if (other.IsHeap())
        {
            m_Heap       = other.m_Heap;
            other.m_Heap = {};
        }
        else
        {
            std::memcpy(m_SSO, other.m_SSO, other.m_Size + 1);
        }

        other.m_Size   = 0;
        other.m_SSO[0] = '\0';
    }

    FString::~FString()
    {
        if (IsHeap())
            delete[] m_Heap.Ptr;
    }

    FString& FString::operator=(const FString& other)
    {
        if (this != &other)
        {
            if (IsHeap())
                delete[] m_Heap.Ptr;

            Assign(other.Data(), other.m_Size);
        }
        return *this;
    }

    FString& FString::operator=(FString&& other) noexcept
    {
        if (this != &other)
        {
            if (IsHeap())
                delete[] m_Heap.Ptr;

            m_Size = other.m_Size;

            if (other.IsHeap())
            {
                m_Heap       = other.m_Heap;
                other.m_Heap = {};
            }
            else
            {
                std::memcpy(m_SSO, other.m_SSO, other.m_Size + 1);
            }

            other.m_Size   = 0;
            other.m_SSO[0] = '\0';
        }
        return *this;
    }

    FString& FString::operator=(const char* str)
    {
        if (IsHeap())
            delete[] m_Heap.Ptr;

        if (str)
            Assign(str, std::strlen(str));
        else
        {
            m_SSO[0] = '\0';
            m_Size   = 0;
        }
        return *this;
    }

    FString& FString::operator=(std::string_view str)
    {
        if (IsHeap())
            delete[] m_Heap.Ptr;

        Assign(str.data(), str.size());
        return *this;
    }

    const char* FString::CStr() const
    {
        return Data();
    }

    std::string_view FString::View() const
    {
        return { Data(), m_Size };
    }

    std::string FString::ToStdString() const
    {
        return { Data(), m_Size };
    }

    FString& FString::InsertBytes(size_t byteOffset, const char* str, size_t length)
    {
        if (!str || length == 0)
            return *this;

        byteOffset = std::min(byteOffset, m_Size);

        size_t newSize   = m_Size + length;
        size_t suffixLen = m_Size - byteOffset;

        if (newSize <= SSO_CAPACITY)
        {
            // Stays in SSO — shift suffix right, copy new bytes in
            std::memmove(m_SSO + byteOffset + length, m_SSO + byteOffset, suffixLen);
            std::memcpy (m_SSO + byteOffset, str, length);
            m_SSO[newSize] = '\0';
        }
        else if (IsHeap())
        {
            if (newSize > m_Heap.Capacity)
            {
                // Heap realloc — allocate, copy prefix / new bytes / suffix, free old
                size_t newCap = std::max(newSize, m_Heap.Capacity * 2);
                char*  newPtr = new char[newCap + 1];
                std::memcpy(newPtr,                       m_Heap.Ptr,             byteOffset);
                std::memcpy(newPtr + byteOffset,          str,                    length);
                std::memcpy(newPtr + byteOffset + length, m_Heap.Ptr + byteOffset, suffixLen);
                newPtr[newSize] = '\0';
                delete[] m_Heap.Ptr;
                m_Heap.Ptr      = newPtr;
                m_Heap.Capacity = newCap;
            }
            else
            {
                // Fits in existing heap allocation — shift suffix in-place
                std::memmove(m_Heap.Ptr + byteOffset + length, m_Heap.Ptr + byteOffset, suffixLen);
                std::memcpy (m_Heap.Ptr + byteOffset, str, length);
                m_Heap.Ptr[newSize] = '\0';
            }
        }
        else
        {
            // SSO → heap transition
            size_t newCap = std::max(newSize, SSO_CAPACITY * 2);
            char*  newPtr = new char[newCap + 1];
            std::memcpy(newPtr,                       m_SSO,             byteOffset);
            std::memcpy(newPtr + byteOffset,          str,               length);
            std::memcpy(newPtr + byteOffset + length, m_SSO + byteOffset, suffixLen);
            newPtr[newSize] = '\0';
            m_Heap.Ptr      = newPtr;
            m_Heap.Capacity = newCap;
        }

        m_Size = newSize;
        return *this;
    }

    FString& FString::InsertBytes(size_t byteOffset, FStringView text)
    {
        return InsertBytes(byteOffset, text.Data(), text.ByteLength());
    }

    FString& FString::EraseBytes(size_t byteOffset, size_t length)
    {
        if (length == 0 || byteOffset >= m_Size)
            return *this;

        length = std::min(length, m_Size - byteOffset);

        size_t newSize   = m_Size - length;
        size_t suffixLen = m_Size - byteOffset - length;

        if (IsHeap() && newSize <= SSO_CAPACITY)
        {
            // Heap → SSO transition
            char* ptr = m_Heap.Ptr;
            std::memcpy(m_SSO,             ptr,                       byteOffset);
            std::memcpy(m_SSO + byteOffset, ptr + byteOffset + length, suffixLen);
            m_SSO[newSize] = '\0';
            delete[] ptr;
        }
        else
        {
            // In-place (SSO or heap) — shift suffix left over the erased region
            char* d = Data();
            std::memmove(d + byteOffset, d + byteOffset + length, suffixLen);
            d[newSize] = '\0';
        }

        m_Size = newSize;
        return *this;
    }

    FString& FString::operator+=(const FString& other)
    {
        AppendBytes(other.Data(), other.m_Size);
        return *this;
    }

    FString& FString::operator+=(const std::string& str)
    {
        AppendBytes(str.data(), str.size());
        return *this;
    }

    FString& FString::operator+=(std::string_view str)
    {
        AppendBytes(str.data(), str.size());
        return *this;
    }

    FString& FString::operator+=(const char* str)
    {
        if (str)
            AppendBytes(str, std::strlen(str));
        return *this;
    }

    FString FString::operator+(const FString& other) const
    {
        FString result(*this);
        result.AppendBytes(other.Data(), other.m_Size);
        return result;
    }

    FString FString::operator+(const std::string& other) const
    {
        FString result(*this);
        result.AppendBytes(other.data(), other.size());
        return result;
    }

    FString FString::operator+(std::string_view other) const
    {
        FString result(*this);
        result.AppendBytes(other.data(), other.size());
        return result;
    }

    FString FString::operator+(const char* other) const
    {
        FString result(*this);
        if (other)
            result.AppendBytes(other, std::strlen(other));
        return result;
    }

    bool FString::operator==(const FString& other) const
    {
        return m_Size == other.m_Size && std::memcmp(Data(), other.Data(), m_Size) == 0;
    }

    bool FString::operator==(const std::string& other) const
    {
        return m_Size == other.size() && std::memcmp(Data(), other.data(), m_Size) == 0;
    }

    bool FString::operator==(std::string_view other) const
    {
        return m_Size == other.size() && std::memcmp(Data(), other.data(), m_Size) == 0;
    }

    bool FString::operator==(const char* other) const
    {
        return other && std::strcmp(Data(), other) == 0;
    }

    SizeT FString::GetHash() const
    {
        return CalculateHash(Data(), m_Size);
    }

    // ------------------------------------------------------------------

    void FString::Assign(const char* str, size_t length)
    {
        m_Size = length;

        if (length <= SSO_CAPACITY)
        {
            std::memcpy(m_SSO, str, length);
            m_SSO[length] = '\0';
        }
        else
        {
            m_Heap.Capacity    = length;
            m_Heap.Ptr         = new char[length + 1];
            std::memcpy(m_Heap.Ptr, str, length);
            m_Heap.Ptr[length] = '\0';
        }
    }

    void FString::AppendBytes(const char* str, size_t length)
    {
        if (length == 0)
            return;

        size_t newSize = m_Size + length;

        if (newSize <= SSO_CAPACITY)
        {
            std::memcpy(m_SSO + m_Size, str, length);
            m_SSO[newSize] = '\0';
        }
        else if (IsHeap())
        {
            if (newSize > m_Heap.Capacity)
            {
                size_t newCap  = std::max(newSize, m_Heap.Capacity * 2);
                char*  newPtr  = new char[newCap + 1];
                std::memcpy(newPtr, m_Heap.Ptr, m_Size);
                std::memcpy(newPtr + m_Size, str, length);
                newPtr[newSize] = '\0';
                delete[] m_Heap.Ptr;
                m_Heap.Ptr      = newPtr;
                m_Heap.Capacity = newCap;
            }
            else
            {
                std::memcpy(m_Heap.Ptr + m_Size, str, length);
                m_Heap.Ptr[newSize] = '\0';
            }
        }
        else
        {
            // SSO -> heap transition
            size_t newCap = std::max(newSize, SSO_CAPACITY * 2);
            char*  newPtr = new char[newCap + 1];
            std::memcpy(newPtr, m_SSO, m_Size);
            std::memcpy(newPtr + m_Size, str, length);
            newPtr[newSize] = '\0';
            m_Heap.Ptr      = newPtr;
            m_Heap.Capacity = newCap;
        }

        m_Size = newSize;
    }

} // namespace Fusion
