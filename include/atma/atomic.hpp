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

		atomic128_t(uint64 a, uint32 b, uint32 c)
		{
			ui64[0] = a;
			ui32[2] = b;
			ui32[3] = c;
		}

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
		struct interlocked_t<T, 1>
		{
			static auto exchange(void* addr, T x) -> T
			{
				auto sx = *reinterpret_cast<CHAR*>(&x);
				return InterlockedExchange8((CHAR*)addr, sx);
			}

			static auto bit_or(T* addr, T x) -> T
			{
				return InterlockedOr8((LONG*)addr, x);
			}
		};

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
			static auto pre_inc(void volatile* addr) -> T
			{
				auto v = InterlockedIncrement((LONG*)addr);
				return *reinterpret_cast<T*>(&v);
			}

			static auto post_inc(void volatile* addr) -> T
			{
				auto v = InterlockedIncrement((LONG*)addr) - 1;
				return *reinterpret_cast<T*>(&v);
			}

			static auto pre_dec(void volatile* addr) -> T
			{
				auto v = InterlockedDecrement((LONG*)addr);
				return *reinterpret_cast<T*>(&v);
			}

			static auto post_dec(void volatile* addr) -> T
			{
				auto v = InterlockedDecrement((LONG*)addr) - 1;
				return *reinterpret_cast<T*>(&v);
			}

			static auto exchange(void* addr, T const& x) -> T
			{
				return InterlockedExchange((LONG*)addr, *reinterpret_cast<LONG const*>(&x));
			}

			static auto compare_exchange(void* addr, T const& c, T const& x, T* outc) -> bool
			{
				auto v = InterlockedCompareExchange((LONG*)addr, x, c);
				bool r = (c == v);
				*outc = v;
				return r;
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

	template <typename T> inline auto atomic_pre_increment(T* addr) -> T { return detail::interlocked_t<T>::pre_inc(addr); }
	template <typename T> inline auto atomic_post_increment(T* addr) -> T { return detail::interlocked_t<T>::post_inc(addr); }
	template <typename T> inline auto atomic_pre_decrement(T* addr) -> T { return detail::interlocked_t<T>::pre_dec(addr); }
	template <typename T> inline auto atomic_post_decrement(T* addr) -> T { return detail::interlocked_t<T>::post_dec(addr); }

	template <typename T>
	inline auto atomic_bitwise_or(T* addr, T x) -> T {
		return detail::interlocked_t<T>::bit_or(addr, x);
	}

	template <typename T>
	inline auto atomic_exchange(void* addr, T const& x) -> T {
		return detail::interlocked_t<T>::exchange(addr, x);
	}

	template <typename T>
	inline auto atomic_compare_exchange(void* addr, T const& c, T const& x) -> bool
	{
		static atomic128_t d;
		return detail::interlocked_t<T>::compare_exchange(addr, c, x, reinterpret_cast<T*>(&d));
	}

	template <typename T>
	inline auto atomic_compare_exchange(void* addr, T const& c, T const& x, T* outc) -> bool
	{
		return detail::interlocked_t<T>::compare_exchange(addr, c, x, outc);
	}

	inline auto atomic_load_128(void* dest, void* src) -> void
	{
		static const atomic128_t t;
		atomic_compare_exchange(src, t, t, reinterpret_cast<atomic128_t*>(dest));
	}

}
