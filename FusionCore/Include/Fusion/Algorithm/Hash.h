#pragma once

#include <algorithm>
#include <functional>

#define FUSION_DECL_HASH(Type)\
	template<>\
	inline SizeT GetHash<Type>(const Type& value)\
	{\
		return std::hash<Type>()(value);\
	}

namespace Fusion
{

	struct Hash128
	{
		u64 high64;
		u64 low64;
	};

	FUSIONCORE_API Hash128 CalculateHash128(const void* data, SizeT length);

	FUSIONCORE_API SizeT CalculateHash(const void* data, SizeT length);

	FUSIONCORE_API SizeT CombineHashes(const SizeT* hashes, SizeT numHashes);

	inline SizeT GetCombinedHash(SizeT hashA, SizeT hashB)
	{
		hashA ^= (hashB + 0x9e3779b9 + (hashA << 6) + (hashA >> 2));
		return hashA;
	}

	/// Default implementation does not have any 'special' code other than for pointers. Specializations do all the work.
	template<typename T>
	SizeT GetHash(const T& value)
	{
		constexpr bool isPointer = std::is_pointer_v<T>;

		typedef std::remove_pointer_t<T> WithoutPtrType;
		typedef TFHasGetHashFunction<T> GetHashFuncType;

		if constexpr (isPointer)
		{
			return (SizeT)(WithoutPtrType*)value;
		}
		else if constexpr (GetHashFuncType::Value)
		{
			return GetHashFuncType::GetHash(&value);
		}
		else
		{
			return std::hash<T>()(value);
		}
	}

	template<typename T>
	SizeT CombineHash(SizeT runningHash, const T& value)
	{
		return GetCombinedHash(runningHash, GetHash<T>(value));
	}


	FUSION_DECL_HASH(u8);
	FUSION_DECL_HASH(u16);
	FUSION_DECL_HASH(u32);
	FUSION_DECL_HASH(u64);

	FUSION_DECL_HASH(i8);
	FUSION_DECL_HASH(i16);
	FUSION_DECL_HASH(i32);
	FUSION_DECL_HASH(i64);

	FUSION_DECL_HASH(f32);
	FUSION_DECL_HASH(f64);

	FUSION_DECL_HASH(bool);
	FUSION_DECL_HASH(char);

	template<typename T>
	struct HashFunc
	{
		SizeT operator()(const T& value) const
		{
			return ::Fusion::GetHash(value);
		}
	};
    
} // namespace Fusion
