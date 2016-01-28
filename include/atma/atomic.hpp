#pragma once

#include <atma/config/platform.hpp>

namespace atma
{
#if ATMA_COMPILER_MSVC
	
	struct alignas(128) atomic128_t
	{
		union {
			int64 i64[2];
			int32 i32[4];
			int16 i16[8];
			int8  i8[16];

			uint64 ui64[2];
			uint32 ui32[4];
			uint16 ui16[8];
			uint8  ui8[16];
		};
	};


	namespace detail
	{
		template <typename T, size_t S = sizeof(T)> struct interlocked_t;

		template <typename T>
		struct interlocked_t<T, 16>
		{
			static auto exchange(void* addr, T x) -> T {
				return InterlockedExchange16((SHORT*)addr, reinterpret_cast<SHORT>(x));
			}
		};

		template <typename T>
		struct interlocked_t<T, 128>
		{
			static auto compare_exchange(void* addr, T const& x) -> T {
				return InterlockedCompareExchange128((LONG64*)addr, x.i64[0], x.i64[1] reinterpret_cast<SHORT>(x));
			}
		};
	}

	template <typename T>
	inline auto atomic_exchange(void* addr, T const& x) -> T {
		return detail::interlocked_t<T>::exchange(addr, x);
	}

	//inline auto atomic_exchange_i32(void* addr, int32 x) -> int32 { return InterlockedExchange((LONG*)addr, reinterpret_cast<LONG>(x)); }
	//inline auto atomic_exchange_u32(void* addr, uint32 x) -> uint32 { return InterlockedExchange((LONG*)addr, reinterpret_cast<LONG>(x));  }
	//
	//inline auto atomic_exchange_i64(void* addr, int64 x) -> int64 { return InterlockedExchange64((LONGLONG*)addr, reinterpret_cast<LONGLONG>(x)); }
	//inline auto atomic_exchange_u64(void* addr, uint64 x) -> uint64 { return InterlockedExchange64((LONGLONG*)addr, reinterpret_cast<LONGLONG>(x)); }
	//
	//inline auto atomic_compare_exchange_i32(void* addr, int32 comparand, int32 x) -> int32 { return InterlockedCompareExchange((LONG*)addr, reinterpret_cast<LONG>(x), reinterpret_cast<LONG>(comparand)); }
	//inline auto atomic_compare_exchange_u32(void* addr, uint32 comparand, uint32 x) -> uint32 { return InterlockedCompareExchange((LONG*)addr, reinterpret_cast<LONG>(x), reinterpret_cast<LONG>(comparand)); }

#endif

	//inline bool atomic_compare_exchange
}
