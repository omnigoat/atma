module;

#include <utility>
#include <type_traits>

export module atma.meta;

// nullptr_v
namespace atma::meta
{
	export template <typename T>
	inline constexpr T* nullptr_v = nullptr;
}

// any_t
namespace atma::meta
{
	template <typename T = void>
	struct any_t_
	{
		constexpr any_t_() = default;

		constexpr any_t_(T&&)
		{}
	};

	template <>
	struct any_t_<void>
	{
		constexpr any_t_() = default;

		template <typename T>
		constexpr any_t_(T&&)
		{}
	};

	export using any_t = any_t_<>;
}

// list
export namespace atma::meta
{
	template <typename... As>
	struct list
	{
		using type = list;
		static constexpr size_t size() noexcept { return sizeof...(As); }
	};


	namespace detail
	{
		template <int N, typename>
		struct drop_;

		template <typename List>
		struct drop_<0, List>
			{ using type = List; };

		template <int N, typename First, typename... Args>
		struct drop_<N, list<First, Args...>>
			{ using type = typename drop_<N - 1, list<Args...>>::type; };
	}

	namespace detail
	{
		template <int N, typename List, typename Result = list<>>
		struct drop_back_ii_;

		template <typename List, typename Result>
		struct drop_back_ii_<0, List, Result>
			{ using type = Result; };

		template <int N, typename First, typename... Args, typename... RArgs>
		struct drop_back_ii_<N, list<First, Args...>, list<RArgs...>>
			{ using type = list<RArgs..., First>; };


		template <int N, typename List>
		struct drop_back_;

		template <int N, typename... Args>
		struct drop_back_<N, list<Args...>>
		{
			static_assert(sizeof...(Args) - N >= 0);
			using type = typename drop_back_ii_<sizeof...(Args) - N, list<Args...>>::type;
		};

		template <typename List>
		struct drop_back_<0, List>
			{ using type = List; };

		template <int N, typename Back, typename... Args>
		struct drop_back_<N, list<Args..., Back>>
			{ using type = typename drop_back_<N - 1, list<Args...>>::type; };
	}

	template <int N, typename list>
	using drop = typename detail::drop_<N, list>::type;

	template <int N, typename list>
	using drop_back = typename detail::drop_back_<N, list>::type;
}

// integral types
export namespace atma::meta
{
	struct nil_ {};

	template <auto x>
	using integral_constant_of = std::integral_constant<decltype(x), x>;

	template <typename...>
	using void_ = void;

	template <bool     x> using bool_   = integral_constant_of<x>;
	template <char     x> using char_   = integral_constant_of<x>;
	template <int      x> using int_    = integral_constant_of<x>;
	template <uint32_t x> using uint32_ = integral_constant_of<x>;
	template <uint64_t x> using uint64_ = integral_constant_of<x>;
}

// integral operations
export namespace atma::meta
{
	template <typename x> using inc = integral_constant_of<++x()>;
	template <typename x> using dec = integral_constant_of<--x()>;

	template <typename x, typename y> using mul = integral_constant_of<x() * y()>;
	template <typename x, typename y> using div = integral_constant_of<x() / y()>;
	template <typename x, typename y> using add = integral_constant_of<x() + y()>;
	template <typename x, typename y> using sub = integral_constant_of<x() - y()>;
}

// identity
export namespace atma::meta
{
	template <typename T>
	struct identity {
		template <typename...> using invoke = T;
		using type = T;
	};
}

// typeval
namespace atma::meta
{
	template <typename T>
	constexpr auto tv_ = T::type::value;
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

	export template <template <typename...> class C, typename... Ts>
	struct defer : detail::defer_<C, Ts...>
	{
	};
}

// invokify & invoke
export namespace atma::meta
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

	export template <typename F, typename List>
	using map = typename map_impl<F, List>::type;
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
		template <typename>
		struct fold_pick;

		template <typename F, typename I, typename... Xs>
		struct fold_pick<list<F, I, list<Xs...>>> : fold_impl<F, I, list<Xs...>>
		{};

		template <typename F, typename XH, typename... Xs>
		struct fold_pick<list<F, list<XH, Xs...>>> : fold_impl<F, XH, list<Xs...>>
		{};
	}

	export template <typename... Args>
	using fold = typename detail::fold_pick<list<Args...>>::type;
}

// bind_back
export namespace atma::meta
{
	template <typename F, typename... Us>
	struct bind_back
	{
		template <typename... Ts>
		using invoke = invoke<F, Ts..., Us...>;
	};
}


// logical operators
export namespace atma::meta
{
	struct and_op {
		template <typename A, typename B>
		using invoke = integral_constant_of<tv_<A> && tv_<B>>;
	};

	struct or_op {
		template <typename A, typename B>
		using invoke = integral_constant_of<tv_<A> || tv_<B>>;
	};
}


// all/any
export namespace atma::meta
{
	template <typename list>
	using all = fold<and_op, bool_<true>, list>;

	template <typename list>
	using any = fold<or_op, bool_<false>, list>;
}

export namespace atma::meta
{
	namespace detail
	{
		template <typename List1, typename List2>
		struct _all_same2_;

		template <typename... Args1, typename... Args2>
		struct _all_same2_<list<Args1...>, list<Args2...>>
		{
			using pair_list = list<std::pair<Args1, Args2>...>;
		};

		template <typename X>
		struct _all_same_
		{
			static_assert(std::is_same_v<X, int>, "wrong");
		};

		template <typename... Args>
		struct _all_same_<list<Args...>>
		{
			using first_list = drop<1, list<Args...>>;
			using second_list = drop_back<1, list<Args...>>;
		};

		template <typename... Args>
		struct _all_same3_
		{
			using L = _all_same_<list<Args...>>;
			using P = typename _all_same2_<typename L::first_list, typename L::second_list>::pair_list;
		};

		struct same_op {
			template <typename X>
			using invoke = std::is_same<typename X::first_type, typename X::second_type>;
		};
	}

	template <typename... Args>
	using all_same = all<map<detail::same_op, typename detail::_all_same3_<Args...>::P>>;
}


