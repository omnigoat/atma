#pragma once

#include <cstdint>
#include <type_traits>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;

typedef uchar byte;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

#define ATMA_PP_CAT_II(a, b) a##b
#define ATMA_PP_CAT(a, b) ATMA_PP_CAT_II(a, b)

#define ATMA_SPLAT_FN_II(counter, pattern) int ATMA_PP_CAT(splat, counter)[] = {0, (pattern, void(), 0)...};
#define ATMA_SPLAT_FN(pattern) ATMA_SPLAT_FN_II(__COUNTER__, pattern)

namespace atma
{
	//
	//  transfer_const_t
	//  ------------------
	//    takes T, and if const, transforms M to be const, otherwise just M
	//
	template <typename T, typename M>
	using transfer_const_t = std::conditional_t<std::is_const<T>::value, M const, M>;

	//
	//  not
	//  -----
	//    negates a predicate type
	//
	template <typename T>
	struct not_v
	{
		static auto const value = !T::value;
		using type = bool;
	};
}
