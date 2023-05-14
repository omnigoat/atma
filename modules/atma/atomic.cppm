module;

#include <atma/config/platform.hpp>
#include <atma/assert.hpp>

#include <atomic>

export module atma.atomic;

import atma.types;

//
// memory_order
// --------------
//
// we are just going to alias to the standard here, as there is no
// point reinventing the wheel.
//
export namespace atma
{
	using memory_order = std::memory_order;

	inline constexpr auto memory_order_acquire = std::memory_order_acquire;
	inline constexpr auto memory_order_release = std::memory_order_release;
	inline constexpr auto memory_order_consume = std::memory_order_consume;
	inline constexpr auto memory_order_relaxed = std::memory_order_relaxed;
	inline constexpr auto memory_order_acq_rel = std::memory_order_acq_rel;
	inline constexpr auto memory_order_seq_cst = std::memory_order_seq_cst;
}


//
// atomic-implementation
// -----------------------
//
// the idea here is that we provide an implementation for every
// <platform, compiler, bitwidth> combination. we first check for a
// specific platform/compiler pair, and if it doesn't exist, fall back
// to an "any-platform"/compiler pair. we do not expect an
// implementation that must be used for a platform, that can also be
// compiled by every compiler.
// 
// a reference implementation is provided to demonstrate the operations.
//
namespace atma::detail::_atomics_
{
	struct any_architecture {};

	// supported platforms
	struct x64 {};
	struct arm64 {};

	// supported compilers
	struct msvc {};
}

namespace atma::detail
{
	template <typename Platform, typename Compiler, size_t Bitwidth>
	struct atomic_implementation
	{
		static_assert(atma::actually_false_v<Platform>, "no relevant atomics-implementation found");
	};
}

namespace atma::detail
{
	template <typename Platform, typename Compiler, size_t Bitwidth, typename = std::void_t<>>
	struct atomic_implementation_chooser
	{
		using type = atomic_implementation<_atomics_::any_architecture, Compiler, Bitwidth>;
	};

	template <typename Platform, typename Compiler, size_t Bitwidth>
	struct atomic_implementation_chooser<
		Platform, Compiler, Bitwidth,
		std::void_t<decltype(atomic_implementation<Platform, Compiler, Bitwidth>)>>
	{
		using type = atomic_implementation<Platform, Compiler, Bitwidth>;
	};

	template <typename Platform, typename Compiler, size_t Bitwidth>
	using atomic_implementation_chooser_t = typename atomic_implementation_chooser<Platform, Compiler, Bitwidth>::type;
}


namespace atma::detail::_atomics_
{
	template <typename Platform, typename Compiler, size_t Bitwidth>
	struct bytes_primitive;

	template <typename Platform, typename Compiler, size_t Bitwidth>
	using bytes_primitive_t = typename bytes_primitive<Platform, Compiler, Bitwidth>::type;

	template <typename Platform, typename Compiler, size_t Bitwidth>
	auto decl_bytes() noexcept
		-> std::add_rvalue_reference_t<bytes_primitive_t<Platform, Compiler, Bitwidth>>
	{
		ATMA_HALT("don't call this! for decltype only.");
	}

	template <typename Platform, typename Compiler, size_t Bitwidth>
	auto decl_bytes_ptr() noexcept
		-> std::add_rvalue_reference_t<std::add_pointer_t<bytes_primitive_t<Platform, Compiler, Bitwidth>>>
	{
		ATMA_HALT("don't call this! for decltype only.");
	}
}

namespace atma::detail::_atomics_
{
	template <typename Platform>
	struct bytes_primitive<Platform, msvc, 1>
		{ using type = __int8; };

	template <typename Platform>
	struct bytes_primitive<Platform, msvc, 2>
		{ using type = __int16; };

	template <typename Platform>
	struct bytes_primitive<Platform, msvc, 4>
		{ using type = __int32; };

	template <typename Platform>
	struct bytes_primitive<Platform, msvc, 8>
		{ using type = __int64; };

#if 0
	template <typename Platform>
	struct bytes_primitive<Platform, _atomics_::msvc, 16>
		{ using type = __int128; };
#endif
}

namespace atma::detail
{
	inline constexpr bool assume_seq_cst = true;
	
	inline bool validate_memory_order(memory_order order)
	{
		return order < memory_order_seq_cst;
	}
}

//
// reference implementation
// --------------------------
//
// this is not atomic-safe or anything.
//
// it's to demonstrate the operations that are taking place
//
namespace atma::detail
{
	struct atomic_reference_implementation
	{
		template <typename T>
		inline static auto load(T const volatile* addr, memory_order) -> T
		{
			return *addr;
		}

		template <typename T>
		inline void store(T volatile* addr, T const& op, memory_order)
		{
			*addr = op;
		}

		template <typename T>
		static auto fetch_add(T volatile* addr, T op, memory_order) -> T
		{
			return (*addr = *addr + op) - op;
		}

		template <typename T>
		static auto fetch_sub(T volatile* addr, T op, memory_order) -> T
		{
			return (*addr = *addr - op) + op;
		}

		template <typename T>
		static auto add(T volatile* addr, T op, memory_order) -> T
		{
			return (*addr = *addr + op);
		}

		template <typename T>
		static auto sub(T volatile* addr, T op, memory_order) -> T
		{
			return (*addr = *addr - op);
		}

		template <typename T>
		inline auto exchange(T volatile* addr, T const& op, memory_order) -> T
		{
			auto r = *addr;
			*addr = op;
			return r;
		}

		template <typename T>
		inline auto compare_and_swap(T volatile* addr, T* expected, T const& replacement) -> bool
		{
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
// necessarily the type being negated, so this addresses that too.
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
// x64 atomic implementation
// ---------------------------
//
// it's important to remember that x64 is _strongly-ordered_. loads
// and stores have inbuilt acquire and release semantics respectively.
// so some of these things don't look atomic at all (like, we're
// ignoring the memory_order parameter).
//
// secondly, x64 is so strongly ordered that there is no re-ordering
// *OTHER THAN* store-load reordering. for those you need a full
// memory barrier. for everything else ~there's mastercard~ there's
// no need.
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
		static bool validate_addresses(Addresses... addresses)
		requires (std::is_pointer_v<Addresses> && ...)
		{
			// a) check the type we're pointing to is 16-bits
			// b) check each address is 16-bit aligned
			return ((sizeof(std::remove_pointer_t<Addresses>) == 2) && ...) && (((uintptr_t)addresses % 2 == 0) && ...);
		}

		template <typename T, bool assume_seq_cst = false>
		static auto load(T const volatile* addr, [[maybe_unused]] memory_order order = memory_order_seq_cst) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));

			__int16 const result = __iso_volatile_load16(reinterpret_cast<__int16 const volatile*>(addr));
			
			if constexpr (assume_seq_cst)
			{
				_Compiler_barrier();
			}
			else switch (order)
			{
				case memory_order::relaxed:
					break;
				case memory_order::consume:
				case memory_order::acquire:
					_Compiler_barrier();
					break;
				case memory_order::release:
				case memory_order::acq_rel:
				default:
					ATMA_HALT("incorrect memory order"); 
			}

			return reinterpret_cast<T const&>(result);
		}

		template <typename T, bool assume_seq_cst = false>
		static void store(T volatile* addr, T const& x, [[maybe_unused]] memory_order order)
		{
			ATMA_ASSERT(validate_addresses(addr));

			__int16 const bytes = atomic_reinterpret_cast<__int16>(x);

			if constexpr (assume_seq_cst)
			{
				InterlockedExchange16(reinterpret_cast<__int16 volatile*>(addr), bytes);
			}
			else switch (order)
			{
				case memory_order::release:
					_Compiler_barrier();
					[[fallthrough]];
				case memory_order::relaxed:
					__iso_volatile_store16(reinterpret_cast<__int16 volatile*>(addr), bytes);
					break;
				case memory_order::consume:
				case memory_order::acquire:
				case memory_order::acq_rel:
				default:
					ATMA_HALT("incorrect memory order");
			}
		}

		template <typename T>
		static auto fetch_add(T volatile* addr, T const& op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_reinterpret_cast<__int16>(op));

			return reinterpret_cast<T const&>(result);
		}

		template <typename T>
		static auto fetch_sub(T volatile* addr, T op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_negate(atomic_reinterpret_cast<__int16>(op)));

			return reinterpret_cast<T const&>(result);
		}

		template <typename T>
		static auto add(T volatile* addr, T op, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const bytes = atomic_reinterpret_cast<__int16>(op);
			__int16 const result = InterlockedExchangeAdd16(
				reinterpret_cast<__int16 volatile*>(addr),
				bytes) + bytes;

			return reinterpret_cast<T const&>(result);
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
		static auto exchange(T volatile* addr, T const& x, [[maybe_unused]] memory_order order) -> T
		{
			ATMA_ASSERT(validate_addresses(addr));
			ATMA_ASSERT(validate_memory_order(order));

			__int16 const result = InterlockedExchange16(
				reinterpret_cast<__int16 volatile*>(addr),
				atomic_reinterpret_cast<__int16>(x));

			return reinterpret_cast<T const&>(result);
		}

		template <typename T>
		static auto compare_and_swap(T volatile* addr, T& expected, T const& replacement,
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

namespace atma::detail::_atomics_
{
	using current_architecture = x64;
	using current_compiler = msvc;
}

namespace atma::detail
{
	template <typename T>
	using specialized_atomic_implementation = atomic_implementation<
		_atomics_::current_architecture,
		_atomics_::current_compiler,
		sizeof(T)>;

	template <typename T>
	using fallback_atomic_implementation = atomic_implementation<
		_atomics_::any_architecture,
		_atomics_::current_compiler,
		sizeof(T)>;

	template <template <typename...> typename Op, typename T>
	using best_atomic_implementation_t = std::conditional_t<
		atma::is_detected_v<Op, _atomics_::current_architecture, _atomics_::current_compiler, T>,
		specialized_atomic_implementation<T>,
		fallback_atomic_implementation<T>>;
}

namespace atma::detail::_atomics_
{
	template <typename Platform, typename Compiler, typename T>
	using load_op = decltype(&detail::atomic_implementation<Platform, Compiler, sizeof(T)>::template load<T, false>);

	template <typename Platform, typename Compiler, typename T>
	using store_op = decltype(&detail::atomic_implementation<Platform, Compiler, sizeof(T)>::template store<T, false>);
}

namespace atma::detail
{
	template <typename T>
	using best_atomic_load_impl_t = best_atomic_implementation_t<_atomics_::load_op, T>;

	template <typename T>
	using best_atomic_store_impl_t = best_atomic_implementation_t<_atomics_::store_op, T>;
}

export namespace atma
{
	template <typename T>
	inline auto atomic_load(T const volatile* address, memory_order mo) -> T
	{
		return detail::best_atomic_load_impl_t<T>::load(address, mo);
	}

	template <typename T>
	inline auto atomic_load(T const volatile* address) -> T
	{
		return detail::best_atomic_load_impl_t<T>::template load<T, true>(address);
	}

	template <typename T>
	inline void atomic_store(T volatile* address, T const& x, memory_order mo)
	{
		return detail::best_atomic_store_impl_t<T>::store(address, x, mo);
	}

	template <typename T>
	inline void atomic_store(T volatile* address, T const& x)
	{
		return detail::best_atomic_store_impl_t<T>::template store<T, detail::assume_seq_cst>(address, x);
	}
}



