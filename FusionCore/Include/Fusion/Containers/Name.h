#pragma once

#include "Fusion/Misc/CoreTypes.h"
#include "Fusion/Algorithm/Hash.h"

#include <string_view>
#include <cstring>

namespace Fusion
{

	// Lightweight string identifier for fast O(1) comparison.
	// Hashed at construction time using xxHash via CalculateHash().
	class FUSIONCORE_API FName
	{
	public:
	    FName() = default;

	    FName(const char* str)
	    {
	        if (str)
	        {
	            m_Hash = CalculateHash(str, std::strlen(str));
				m_String = str;
	        }
	    }

	    FName(const FString& str)
	    {
		    if (!str.Empty())
		    {
	            m_Hash = str.GetHash();
				m_String = str;
		    }
	    }

	    FName(std::string_view str)
	    {
	        if (!str.empty())
	        {
	            m_Hash = CalculateHash(str.data(), str.size());
				m_String = str;
	        }
	    }

		FName(const FName& other) = default;
		FName& operator=(const FName& other) = default;

		FName(FName&& other) = default;
		FName& operator=(FName&& other) = default;

	    SizeT GetHash()  const { return m_Hash; }
	    bool  IsValid()  const { return m_Hash != 0; }
	    bool  IsNull()   const { return m_Hash == 0; }

	    explicit operator bool() const { return IsValid(); }

		const FString& ToString() const { return m_String; }

	    bool operator==(const FName& other) const { return m_Hash == other.m_Hash; }
	    bool operator!=(const FName& other) const { return !(*this == other); }

	private:
	    SizeT m_Hash = 0;
		FString m_String{};
	
	};

	template<>
	inline SizeT GetHash<FName>(const FName& name)
	{
	    return name.GetHash();
	}

} // namespace Fusion

// Allow FString to be used directly as a std::format argument.
// Example: FString::Format("Value: {}", someString)
template<>
struct std::formatter<Fusion::FName> : std::formatter<std::string_view>
{
	auto format(const Fusion::FName& name, std::format_context& ctx) const
	{
		return std::formatter<std::string_view>::format(name.ToString().ToStdString(), ctx);
	}
};
