module;

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>

#include <atomic>

export module atma.atomic;

#define ATMA_ASSERT_16BIT_ALIGNED(addr) ATMA_ASSERT((uint64)addr % 2 == 0, "not aligned to 2-byte boundary")
#define ATMA_ASSERT_32BIT_ALIGNED(addr) ATMA_ASSERT((uint64)addr % 4 == 0, "not aligned to 4-byte boundary")
#define ATMA_ASSERT_64BIT_ALIGNED(addr) ATMA_ASSERT((uint64)addr % 8 == 0, "not aligned to 8-byte boundary")


namespace atma
{
	using memory_order = std::memory_order;

	inline constexpr auto memory_order_acquire = std::memory_order_acquire;
	inline constexpr auto memory_order_release = std::memory_order_release;
	inline constexpr auto memory_order_consume = std::memory_order_consume;
	inline constexpr auto memory_order_relaxed = std::memory_order_relaxed;
	inline constexpr auto memory_order_acq_rel = std::memory_order_acq_rel;
	inline constexpr auto memory_order_seq_cst = std::memory_order_seq_cst;
}

namespace atma::detail::_atomics_
{
	// supported platforms
	struct reference_implementation {};
	struct x64 {};

	// supported compilers
	struct msvc {};
}

namespace atma::detail
{
	template <typename Platform, typename Compiler, size_t BitWidth>
	struct atomic_implementation;
}

namespace atma::detail
{
	inline bool validate_memory_order(memory_order order)
	{
		return order < memory_order_seq_cst;
	}
}

//
//
// reference implementation
// --------------------------
// 
// this is not atomic-safe or anything. it's to demonstrate the operations
// that are taking place
//
//
namespace atma::detail
{
	template <typename Compiler, size_t BitSize>
	struct atomic_implementation<_atomics_::reference_implementation, Compiler, BitSize>
	{
		template <typename... Addresses>
		inline static bool validate_addresses(Addresses... addresses)
		requires (std::is_pointer_v<Addresses> && ...)
		{
			// a) check the type we're pointing to is BitSize
			// b) check each address is BitSize aligned
			return ((sizeof(std::remove_pointer_t<Addresses>) == BitSize) && ...) && ((addresses % BitSize == 0) && ...);
		}

		template <typename T>
		inline static auto load(T const volatile* addr, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			return *addr;
		}

		template <typename T>
		inline void store(T volatile* addr, T const& op, memory_order)
		{
			ATMA_ASSERT(validate_addresses(addr));
			*addr = op;
		}

		template <typename T>
		static auto fetch_add(T volatile* addr, T op, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			return (*addr = *addr + op) - op;
		}

		template <typename T>
		static auto fetch_sub(T volatile* addr, T op, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			return (*addr = *addr - op) + op;
		}

		template <typename T>
		static auto add(T volatile* addr, T op, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			return (*addr = *addr + op);
		}

		template <typename T>
		static auto sub(T volatile* addr, T op, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			return (*addr = *addr - op);
		}

		template <typename T>
		inline auto exchange(T volatile* addr, T const& op, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			auto r = *addr;
			*addr = op;
			return r;
		}

		template <typename T>
		inline auto compare_and_swap(T volatile* addr, T* expected, T const& replacement) -> bool
		{
			ATMA_ASSERT(validate_addresses(addr, expected));

			// bitwise comparison required, can't use operator == (T, T)
			if (auto r = *reinterpret_cast<uint16_t volatile*>(addr); r == *reinterpret_cast<uint16_t*>(expected))
			{
				*addr = replacement;
				return true;
			}
			else
			{
				*expected = r;
				return false;
			}
		}
	};
}


//
// atomic_reinterpret_cast
// -------------------------
// 
// (openly stolen from microsoft's standard library implementation.)
//
// the hidden requirement here that necessitates this is the zeroing
// out of padding bits of non-integral types. so zero-initializing an
// integral and memcpying into it seems best.
//
// we have placed a further restriction requiring the size of the
// type & the integral to match exactly
//
namespace atma::detail
{
	template <std::integral Integral, class SourceType>
	[[nodiscard]] Integral atomic_reinterpret_cast(SourceType const& source) noexcept
	requires (sizeof(Integral) == sizeof(SourceType))
	{
		if constexpr (std::is_integral_v<SourceType>)
		{
			return static_cast<Integral>(source);
		}
		else if constexpr (std::is_pointer_v<SourceType>)
		{
			return reinterpret_cast<Integral>(source);
		}
		else
		{
			Integral result{};
			std::memcpy(&result, std::addressof(source), sizeof(source));
			return result;
		}
	}
}

//
// atomic_negate
// ---------------
// 
// even though c++20 defines all integers to be in two's complement,
// we still run into a pickle when we need to negate INT_MIN, as i.e.,
// -128 can not be negated to +128 within a byte.
// 
// this function will perform unsigned subtraction from zero, which
// for all non-INT_MIN values will result in exactly the same as
// regular negation (-x) would, but for INT_MIN will result, with
// maths, as INT_MIN again
// 
// also lol the negate operator (-x) returns a straight-up int, not
// necessarily the type being negated...
//
namespace atma::detail
{
	template <std::integral T>
	[[nodiscard]] static auto atomic_negate(T const& x) noexcept
	{
		return static_cast<T>(0U - static_cast<std::make_unsigned_t<T>>(x));
	}
}

//
//
// x64 atomic implementation
// ---------------------------
// 
// it's important to remember that x64 is _strong-ordered_. loads
// and stores have inbuilt acquire and release semantics respectively.
// so some of these things don't look atomic at all (like, we're
// ignoring the memory_order parameter).
//
// secondly, x64 is so strongly ordered that there is no re-ordering
// *OTHER THAN* store-load reordering. for those you need a full
// memory barrier. for everything else ~there's mastercard~ there's
// no need
//
//
// msvc: Interlocked functions all act as full memory barriers.
//       versions postfixed with Acquire, Release, or NoFence, are
//       all less barriery than that. this means that many of our
//       implementations don't look at the memory_order, as by
//       performing an Interlocked on x64 hardware, we are implicitly
//       sequentially-consistent, the strongest of guarantees.
//
namespace atma::detail
{
	template <>
	struct atomic_implementation<_atomics_::x64, _atomics_::msvc, 2>
	{
		template <typename... Addresses>
		inline static bool validate_addresses(Addresses... addresses)
		requires (std::is_pointer_v<Addresses> && ...)
		{
			// a) check the type we're pointing to is 16-bits
			// b) check each address is 16-bit aligned
			return ((sizeof(std::remove_pointer_t<Addresses>) == 2) && ...) && ((addresses % 2 == 0) && ...);
		}

		template <typename T, bool assume_seq_cst = false>
		inline static auto load(T const volatile* addr, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));

			__int16 const result = __iso_volatile_load16(reinterpret_cast<__int16 const volatile*>(addr));
			
			if constexpr (assume_seq_cst)
			{
				_Compiler_or_memory_barrier();
			}
			else switch (order)
			{
				case memory_order::relaxed:
					break;
				case memory_order::consume:
				case memory_order::acquire:
					_Compiler_or_memory_barrier();
				case memory_order::release:
				case memory_order::acq_rel:
				default:
					ATMA_HALT("incorrect memory order"); 
			}

			return reinterpret_cast<T const&>(result);
		}

		template <typename T, bool assume_seq_cst = false>
		inline void store(T volatile* addr, T const& x, [[maybe_unused]] memory_order order)
		{
			ATMA_ASSERT(validate_addresses(addr));

			__int16 const bytes = atomic_reinterpret_cast<__int16>(x);

			if constexpr (assume_seq_cst)
			{
				InterlockedExchange16(reinterpret_cast<__int16 volatile*>(addr), bytes);
			}
			else
			{
				__iso_volatile_store16(reinterpret_cast<__int16 volatile*>(addr), bytes);

				switch (order)
				{
					case memory_order::relaxed:
						break;
					case memory_order::release:
						_Compiler_barrier();
					case memory_order::consume:
					case memory_order::acquire:
					case memory_order::acq_rel:
					default:
						ATMA_HALT("incorrect memory order");
				}
			}
		}

		template <typename T>
		static auto fetch_add(T volatile* addr, T const& op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_reinterpret_cast<__int16>(op));

			return reinterpret_cast<T&>(result);
		}

		template <typename T>
		static auto fetch_sub(T volatile* addr, T op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_negate(atomic_reinterpret_cast<__int16>(op)));

			return reinterpret_cast<T&>(result);
		}

		template <typename T>
		static auto add(T volatile* addr, T op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 bytes = atomic_reinterpret_cast<__int16>(op);
			__int16 result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				bytes) + bytes;

			return reinterpret_cast<T&>(result);
		}

		template <typename T>
		static auto sub(T volatile* addr, T op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const bytes = atomic_reinterpret_cast<__int16>(op);
			__int16 const result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_negate(bytes));

			__int16 const postresult = result - bytes;

			return reinterpret_cast<T const&>(postresult);
		}

		template <typename T>
		inline auto exchange(T volatile* addr, T const& x, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const result = InterlockedExchange16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_reinterpret_cast<__int16>(x));

			return reinterpret_cast<T const&>(result);
		}

		template <typename T>
		inline auto compare_and_swap(T volatile* addr, T& expected, T const& replacement,
			[[maybe_unused]] memory_order order_success, [[maybe_unused]] memory_order order_failure) -> bool
		{
			ATMA_ASSERT(validate_addresses(addr, expected));
			ATMA_ASSERT(validate_memory_order(order_success));
			ATMA_ASSERT(validate_memory_order(order_failure));

			__int16 const expected_bytes = atomic_reinterpret_cast<__int16>(expected);
			__int16 const replacement_bytes = atomic_reinterpret_cast<__int16>(replacement);

			__int16 const previous_bytes = InterlockedCompareExchange16(
				reinterpret_cast<__int16 volatile*>(addr),
				replacement_bytes,
				expected_bytes);
			
			if (previous_bytes == expected_bytes)
			{
				return true;
			}
			else
			{
				std::memcpy(std::addressof(expected), &previous_bytes, sizeof(T));
				return false;
			}
		}
	};
}

export namespace atma
{
	template <typename T>
	T atomic_load(T const volatile* address, memory_order mo = memory_order_seq_cst)
	{
		return detail::atomic_implementation<detail::_atomics_::x64, detail::_atomics_::msvc, sizeof(T)>
			::load(address, mo);
	}
}


#if !defined(ATMA_COMPILER_MSVC)

	#if defined(InterlockedIncrement16)
		#error Interlocked functions already defined??
	#endif

	#define InterlockedIncrement16 _InterlockedIncrement16
	#define InterlockedIncrementAcquire16 _InterlockedIncrement16
	#define InterlockedIncrementRelease16 _InterlockedIncrement16
	#define InterlockedIncrementNoFence16 _InterlockedIncrement16
	#define InterlockedDecrement16 _InterlockedDecrement16
	#define InterlockedDecrementAcquire16 _InterlockedDecrement16
	#define InterlockedDecrementRelease16 _InterlockedDecrement16
	#define InterlockedDecrementNoFence16 _InterlockedDecrement16
	#define InterlockedCompareExchange16 _InterlockedCompareExchange16
	#define InterlockedCompareExchangeAcquire16 _InterlockedCompareExchange16
	#define InterlockedCompareExchangeRelease16 _InterlockedCompareExchange16
	#define InterlockedCompareExchangeNoFence16 _InterlockedCompareExchange16

#endif





