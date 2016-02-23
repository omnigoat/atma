#pragma once

#include <atma/config/platform.hpp>

namespace atma
{
	struct alignas(16) atomic128_t
	{
		atomic128_t()
			: ui64{0, 0}
		{}

		atomic128_t(uint64 a, uint64 b)
			: ui64{a, b}
		{}

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

#if ATMA_COMPILER_MSVC

	namespace detail
	{
		template <typename T, size_t S = sizeof(T)> struct interlocked_t;

		template <typename T>
		struct interlocked_t<T, 2>
		{
			static auto exchange(void* addr, T x) -> T
			{
				auto sx = *reinterpret_cast<SHORT*>(&x);
				return InterlockedExchange16((SHORT*)addr, sx);
			}
		};

		template <typename T>
		struct interlocked_t<T, 4>
		{
			static auto exchange(void* addr, T const& x) -> T
			{
				return InterlockedExchange((LONG*)addr, *reinterpret_cast<LONG*>(&x));
			}

			static auto compare_exchange(void* addr, T const& c, T const& x, T* outc) -> bool
			{
				// reinterpret c & x as an atomic128 for convenience
				*outc = InterlockedCompareExchange((LONG*)addr, x, c);
				return *outc == c;
			}
		};

		template <typename T>
		struct interlocked_t<T, 8>
		{
			static auto exchange(void* addr, T const& x) -> T
			{
				return InterlockedExchange64((LONG64*)addr, *reinterpret_cast<LONG64*>(&x));
			}

			static auto compare_exchange(void* addr, T const& c, T const& x, T* outc) -> bool
			{
				// reinterpret c & x as an atomic128 for convenience
				*reinterpret_cast<LONG64*>(outc) = InterlockedCompareExchange64((LONG64*)addr, *(LONG64*)&x, *(LONG64*)&c);
				return *outc == c;
			}
		};

		template <typename T>
		struct interlocked_t<T, 16>
		{
			static auto compare_exchange(void* addr, T const& c, T const& x, T* outc) -> bool
			{
				// reinterpret c & x as an atomic128 for convenience
				//auto ac = *reinterpret_cast<atomic128_t const*>(&c);
				auto const& ax = *reinterpret_cast<atomic128_t const*>(&x);
				*outc = c;

				return InterlockedCompareExchange128((LONG64*)addr, ax.i64[1], ax.i64[0], (LONG64*)outc) != 0;
			}
		};
	}

#endif


	template <typename T>
	inline auto atomic_exchange(void* addr, T const& x) -> T {
		return detail::interlocked_t<T>::exchange(addr, x);
	}

	template <typename T>
	inline auto atomic_compare_exchange(void* addr, T const& c, T const& x, T* outc = nullptr) -> bool
	{
		static atomic128_t d;

		if (!outc)
			outc = (T*)&d;

		return detail::interlocked_t<T>::compare_exchange(addr, c, x, outc);
	}

	inline auto atomic_read128(void* dest, void* src) -> void
	{
		auto adest = reinterpret_cast<atomic128_t*>(dest);
		atomic_compare_exchange(src, *adest, *adest, adest);
	}

}
