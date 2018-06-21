#pragma once

#include <functional>
#include <utility>
#include <algorithm>

// nullptr_v
namespace atma::meta
{
	template <typename T>
	inline constexpr T* nullptr_ = nullptr;
}

// any
namespace atma::meta
{
	template <typename T = void>
	struct any_
	{
		constexpr any_() = default;
		constexpr any_(T&&)
		{}
	};

	template <>
	struct any_<void>
	{
		constexpr any_() = default;
		template <typename T>
		constexpr any_(T&&)
		{}
	};

	using any = any_<>;
}

// list
namespace atma::meta
{
	template <typename... As>
	struct list
	{
		using type = list;
		static constexpr size_t size() noexcept { return sizeof...(As); }
	};
}

// integral types
namespace atma::meta
{
	struct nil_ {};

	template <auto x>
	using integral_constant_of = std::integral_constant<decltype(x), x>;

	template <bool     x> using bool_   = integral_constant_of<x>;
	template <char     x> using char_   = integral_constant_of<x>;
	template <int      x> using int_    = integral_constant_of<x>;
	template <uint32_t x> using uint32_ = integral_constant_of<x>;
	template <uint64_t x> using uint64_ = integral_constant_of<x>;
}

// integral operations
namespace atma::meta
{
	template <typename x> using inc = integral_constant_of<++x()>;
	template <typename x> using dec = integral_constant_of<--x()>;

	template <typename x, typename y> using mul = integral_constant_of<x() * y()>;
	template <typename x, typename y> using div = integral_constant_of<x() / y()>;
	template <typename x, typename y> using add = integral_constant_of<x() + y()>;
	template <typename x, typename y> using sub = integral_constant_of<x() - y()>;
}

// identity
namespace atma::meta
{
	template <typename T>
	struct identity {
		template <typename...> using invoke = T;
		using type = T;
	};
}

// defer
namespace atma::meta
{
	namespace detail
	{
		template <template <typename...> typename C, typename... Ts, template <typename...> typename D = C>
		auto try_defer_(int) ->
			identity<D<Ts...>>;

		template <template <typename...> typename C, typename... Ts>
		auto try_defer_(long) ->
			nil_;

		template <template <typename...> class C, typename... Ts>
		using defer_ = decltype(try_defer_<C, Ts...>(0));
	}

	template <template <typename...> typename C, typename... Ts>
	struct defer : decltype(detail::try_defer_<C, Ts...>(0))
	{};
}

// invokify & invoke
namespace atma::meta
{
	template <template <typename...> typename C>
	struct invokify {
		template <typename... Ts>
		using invoke = detail::defer_<C, Ts...>;
	};


	template <typename F, typename... Args>
	using invoke = typename F::template invoke<Args...>;
}

// map
namespace atma::meta
{
	namespace detail
	{
		template <typename, typename>
		struct map_impl;

		template <class F>
		struct map_impl<F, list<>> {
			using type = list<>;
		};

		template <typename F, typename... Xs>
		struct map_impl<F, list<Xs...>> {
			using type = list<invoke<F, Xs>...>;
		};
	}

	template <typename F, typename List>
	using map = typename detail::map_impl<F, List>::type;
}

// fold
namespace atma::meta
{
	namespace detail
	{
		template <typename, typename, typename>
		struct fold_impl;

		template <typename F, typename I>
		struct fold_impl<F, I, list<>> {
			using type = I;
		};

		template <typename F, typename I, typename XH, typename... Xs>
		struct fold_impl<F, I, list<XH, Xs...>> {
			using type = typename fold_impl<F, invoke<F, I, XH>, list<Xs...>>::type;
		};


		// fold_pick: pick which fold function to use (init value or no)
		template <typename Args>
		struct fold_pick;

		template <typename F, typename I, typename... Xs>
		struct fold_pick<list<F, I, list<Xs...>>> : fold_impl<F, I, list<Xs...>>
		{};

		template <typename F, typename XH, typename... Xs>
		struct fold_pick<list<F, list<XH, Xs...>>> : fold_impl<F, XH, list<Xs...>>
		{};
	}

	template <typename... Args>
	using fold = typename detail::fold_pick<list<Args...>>::type;
}

// bind_back
namespace atma::meta
{
	template <typename F, typename... Us>
	struct bind_back
	{
		template <typename... Ts>
		using invoke = invoke<F, Ts..., Us...>;
	};
}


// and_op
namespace atma::meta
{
	struct and_op {
		template <typename x, typename y>
		using invoke = integral_constant_of<x() && y()>;
	};

	template <typename... xs>
	using all = fold<and_op, bool_<true>, list<xs...>>;

	template <typename... xs>
	inline constexpr bool all_v = all<xs...>::value;
}




