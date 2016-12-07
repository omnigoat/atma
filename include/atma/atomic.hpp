#pragma once

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>

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

		static_assert(sizeof(uint64) == sizeof(uintptr), "bad sizes");

		union
		{
			intptr iptr[2];
			int64  i64[2];
			int32  i32[4];
			int16  i16[8];
			int8   i8[16];

			uintptr uptr[2];
			uint64  ui64[2];
			uint32  ui32[4];
			uint16  ui16[8];
			uint8   ui8[16];
		};
	};


	using memory_order = std::memory_order;



#if ATMA_COMPILER_MSVC

#define ADDR_CAST(type, x) reinterpret_cast<type*>(x)
#define VALUE_CAST(type, x) reinterpret_cast<type&>(x)

#define ATMA_ASSERT_32BIT_ALIGNED(addr) ATMA_ASSERT((uint64)addr % 4 == 0, "not aligned to 4-byte boundary")
#define ATMA_ASSERT_64BIT_ALIGNED(addr) ATMA_ASSERT((uint64)addr % 8 == 0, "not aligned to 8-byte boundary")

	namespace detail
	{
		template <typename D, typename S, size_t Sz = sizeof(D) * 8>
		struct interlocked_impl_t;

		template <typename D, typename S>
		struct interlocked_impl_t<D, S, 8>
		{
			static auto exchange(D* addr, S x) -> D
			{
				return reinterpret_cast<D>(InterlockedExchange8((CHAR*)addr, reinterpret_cast<CHAR const&>(x)));
			}

			static auto bit_or(D* addr, S x) -> D
			{
				return InterlockedOr8((LONG*)addr, x);
			}
		};

		template <typename D, typename S>
		struct interlocked_impl_t<D, S, 16>
		{
			static auto pre_inc(void volatile* addr) -> D
			{
				auto v = InterlockedIncrement16((SHORT*)addr);
				return *reinterpret_cast<D*>(&v);
			}

			static auto post_inc(void volatile* addr) -> D
			{
				auto v = InterlockedIncrement16((SHORT*)addr) - 1;
				return *reinterpret_cast<D*>(&v);
			}

			static auto pre_dec(void volatile* addr) -> D
			{
				auto v = InterlockedDecrement16((SHORT*)addr);
				return *reinterpret_cast<D*>(&v);
			}

			static auto post_dec(void volatile* addr) -> D
			{
				auto v = InterlockedDecrement16((SHORT*)addr) - 1;
				return *reinterpret_cast<D*>(&v);
			}

			static auto exchange(void* addr, D x) -> D
			{
				auto sx = *reinterpret_cast<SHORT*>(&x);
				return InterlockedExchange16((SHORT*)addr, sx);
			}

			static auto compare_exchange(D volatile* addr, D const& c, S const& x, D* outc) -> bool
			{
				*ADDR_CAST(SHORT, outc) = InterlockedCompareExchange16(
					ADDR_CAST(SHORT volatile, addr),
					VALUE_CAST(SHORT const, x),
					VALUE_CAST(SHORT const, c));

				return *outc == c;
			}
		};

		

		template <typename D, typename S>
		struct interlocked_impl_t<D, S, 32>
		{
			static auto load(D volatile* dest, S volatile* addr, memory_order order = memory_order::memory_order_seq_cst) -> void
			{
				ATMA_ASSERT_32BIT_ALIGNED(addr);
				// loads from 4-byte aligned addresses are atomic on x86/x64
				*reinterpret_cast<uint32 volatile*>(dest) = *reinterpret_cast<uint32 volatile*>(addr);
				_ReadWriteBarrier();
				// no fencing required on x86/x64
			}

			static auto store(D volatile* addr, S const& x) -> void
			{
				InterlockedExchange(
					ADDR_CAST(LONG volatile, addr),
					VALUE_CAST(LONG const, x));
			}

			static auto pre_inc(D volatile* addr) -> D
			{
				auto v = InterlockedIncrement(ADDR_CAST(LONG volatile, addr));
				return VALUE_CAST(D, v);
			}

			static auto post_inc(D volatile* addr) -> D
			{
				auto v = InterlockedIncrement(ADDR_CAST(LONG volatile, addr)) - 1;
				return VALUE_CAST(D, v);
			}

			static auto pre_dec(D volatile* addr) -> D
			{
				auto v = InterlockedDecrement(ADDR_CAST(LONG volatile, addr));
				return VALUE_CAST(D, v);
			}

			static auto post_dec(D volatile* addr) -> D
			{
				auto v = InterlockedDecrement(ADDR_CAST(LONG volatile, addr)) - 1;
				return VALUE_CAST(D, v);
			}

			static auto add(D volatile* addr, S const& x) -> D
			{
				auto v = InterlockedAdd(ADDR_CAST(LONG volatile, addr), VALUE_CAST(D const, x));
				return VALUE_CAST(D, v);
			}

			static auto exchange(D volatile* addr, S const& x) -> D
			{
				auto r = InterlockedExchange(
					ADDR_CAST(LONG volatile, addr),
					VALUE_CAST(LONG const, x));

				return VALUE_CAST(D, r);
			}

			static auto compare_exchange(D volatile* addr, D const& c, S const& x, D* outc) -> bool
			{
				auto v = InterlockedCompareExchange(
					ADDR_CAST(LONG volatile, addr),
					VALUE_CAST(LONG const, x),
					VALUE_CAST(LONG const, c));

				bool r = v == c;

				if (!r)
					*outc = v;

				return r;
			}
		};

		template <typename D, typename S>
		struct interlocked_impl_t<D, S, 64>
		{
			static auto load(D volatile* dest, S volatile* addr) -> void
			{
				ATMA_ASSERT_64BIT_ALIGNED(addr);
				// loads from 4-byte aligned addresses are atomic on x86/x64
				_ReadWriteBarrier(); 
				*reinterpret_cast<uint64 volatile*>(dest) = *reinterpret_cast<uint64 volatile*>(addr);
				_ReadWriteBarrier();
				// no fencing required on x86/x64
			}

			static auto exchange(D volatile* addr, S const& x) -> D
			{
				auto v = InterlockedExchange64(
					ADDR_CAST(LONG64 volatile, addr),
					VALUE_CAST(LONG64 const, x));

				return VALUE_CAST(D, v);
			}

			static auto compare_exchange(D volatile* addr, D const& c, S const& x, D* outc) -> bool
			{
				auto v = InterlockedCompareExchange64(
					ADDR_CAST(LONG64 volatile, addr),
					VALUE_CAST(LONG64 const, x),
					VALUE_CAST(LONG64 const, c));

				bool r = reinterpret_cast<LONG64&>(v) == reinterpret_cast<LONG64 const&>(c);

				if (!r)
					*reinterpret_cast<LONG64*>(outc) = v;

				return r;
			}
		};

		template <typename D, typename S>
		struct interlocked_impl_t<D, S, 128>
		{
			static auto exchange(D volatile* addr, S const& x) -> D
			{
				atomic128_t r;
				while (!compare_exchange(addr, (D&)*addr, x, &r));
				return VALUE_CAST(D, r);
			}

			static auto compare_exchange(D volatile* addr, D const& c, S const& x, D* outc) -> bool
			{
				// the 128-vit version of InterlockedCompareExchange in/outs the comparison/dest value
				*outc = c;

				auto const& ax = VALUE_CAST(atomic128_t const, x);

				return InterlockedCompareExchange128(
					ADDR_CAST(LONG64 volatile, addr),
					ax.i64[1], ax.i64[0],
					ADDR_CAST(LONG64, outc)) != 0;
			}
		};


		template <typename D, typename S = D>
		struct interlocked_t : interlocked_impl_t<D, S, sizeof(D) * 8>
		{
			static_assert(sizeof(D) == sizeof(S), "bad sizes!");
		};
	}

#undef ADDR_CAST
#undef VALUE_CAST

#endif

	template <typename T> inline auto atomic_pre_increment(T* addr) -> T { return detail::interlocked_t<T>::pre_inc(addr); }
	template <typename T> inline auto atomic_post_increment(T* addr) -> T { return detail::interlocked_t<T>::post_inc(addr); }
	template <typename T> inline auto atomic_pre_decrement(T* addr) -> T { return detail::interlocked_t<T>::pre_dec(addr); }
	template <typename T> inline auto atomic_post_decrement(T* addr) -> T { return detail::interlocked_t<T>::post_dec(addr); }

	template <typename D, typename S>
	inline auto atomic_add(D volatile* addr, S const& x) -> D
	{
		return detail::interlocked_t<D, S>::add(addr, x);
	}

	template <typename D, typename S>
	inline auto atomic_bitwise_or(D volatile* dest, S x) -> D {
		return detail::interlocked_t<D, S>::bit_or(dest, x);
	}

	template <typename D, typename S>
	inline auto atomic_exchange(D volatile* addr, S const& x) -> D {
		return detail::interlocked_t<D, S>::exchange(addr, x);
	}

	//template <typename T>
	//inline auto atomic_store(void* addr, T const& x) -> void {
	//	atomic_exchange(addr, x);
	//}

	template <typename D, typename S>
	inline auto atomic_compare_exchange(D volatile* addr, D const& c, S const& x) -> bool
	{
		static atomic128_t d;
		return detail::interlocked_t<D, S>::compare_exchange(addr, c, x, reinterpret_cast<D*>(&d));
	}

	template <typename D, typename S>
	inline auto atomic_compare_exchange(D volatile* addr, D const& c, S const& x, D* outc) -> bool
	{
		return detail::interlocked_t<D, S>::compare_exchange(addr, c, x, outc);
	}

	template <typename D, typename S>
	inline auto atomic_load(D* dest, S volatile* addr, memory_order = memory_order::memory_order_seq_cst) -> void
	{
		detail::interlocked_t<D, S>::load(dest, addr);
	}

	template <typename D, typename S>
	inline auto atomic_store(D volatile* addr, S const& x) -> void
	{
		detail::interlocked_t<D, S>::store(addr, x);
	}

	template <typename D, typename S>
	inline auto atomic_load_128(D* dest, S volatile* src) -> void
	{
		static const atomic128_t t;
		atomic_compare_exchange<S, D>(
			src,
			reinterpret_cast<D const&>(t),
			reinterpret_cast<S const&>(t),
			dest);
	}

}
