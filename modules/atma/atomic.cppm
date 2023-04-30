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
	template <typename Compiler>
	struct atomic_implementation<_atomics_::reference_implementation, Compiler, 2>
	{
		template <typename... Types, typename... Addresses>
		inline static bool validate_addresses(Addresses... addresses)
		requires (std::is_pointer_v<Addresses> && ...)
		{
			static_assert(((sizeof(Types) == 2) && ...),
				"sizeof type is not 16-bits. sadness ensues");

			return ((sizeof(Types) == 2) && ...) && ((addresses % 2 == 0) && ...);
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
		inline auto compare_and_swap(T volatile* addr, T* cmp, T const& val) -> bool
		{
			ATMA_ASSERT(validate_addresses(addr, cmp));

			// bitwise comparison required, can't use operator == (T, T)
			if (auto r = *reinterpret_cast<uint16_t volatile*>(addr); r == *reinterpret_cast<uint16_t*>(cmp))
			{
				*addr = val;
				return true;
			}
			else
			{
				*cmp = r;
				return false;
			}
		}
	};
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
namespace atma::detail
{
	template <>
	struct atomic_implementation<_atomics_::x64, _atomics_::msvc, 2>
	{
		template <typename... Types, typename... Addresses>
		inline static bool validate_addresses(Addresses... addresses)
		requires (std::is_pointer_v<Addresses> && ...)
		{
			static_assert(((sizeof(Types) == 2) && ...),
				"sizeof type is not 16-bits. sadness ensues");

			return ((sizeof(Types) == 2) && ...) && (((uintptr_t)addresses % 2 == 0) && ...);
		}

		template <typename T>
		inline static auto load(T const volatile* addr, memory_order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			// aligned loads are atomic on x64
			__int16 r = __iso_volatile_load16(reinterpret_cast<__int16 const volatile*>(addr));
			return reinterpret_cast<T&>(r);
		}

		template <typename T>
		inline void store(T volatile* addr, T const& op, memory_order)
		{
			ATMA_ASSERT(validate_addresses(addr));
			// aligned writes are atomic on x64
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
		inline auto compare_and_swap(T volatile* addr, T* cmp, T const& val) -> bool
		{
			ATMA_ASSERT(validate_addresses(addr, cmp));

			// bitwise comparison required, can't use operator ==
			//if (auto r = *reinterpret_cast<uint16_t volatile*>(addr); r == *reinterpret_cast<uint16_t*>(cmp))
			if (auto r = *addr; _interlockedbittestandset(addr, *cmp))
			{
				*addr = val;
				return true;
			}
			else
			{
				*cmp = r;
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





