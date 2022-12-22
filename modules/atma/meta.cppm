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

// type
export namespace atma::meta
{
	template <typename T>
	struct type
	{
		using value_type = T;
	};
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

	struct listify
	{
		template <typename... Ts>
		using apply = list<Ts...>;
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
	struct nothing {};

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
export namespace atma::meta::lazy
{
	struct identity
	{
		template <typename T>
		using apply = T;
	};
}

export namespace atma::meta
{
	template <typename T>
	using identity = T;
}

// typeval
namespace atma::meta
{
	template <typename T>
	constexpr auto tv_ = T::type::value;
}


// defer
namespace atma::meta::lazy::detail
{
	template <bool, typename>
	struct _defer_
	{};

	template <typename F>
	struct _defer_<true, F> : F
	{};
}

// call-continuation
// -------------------
//  yeah requires explanation
export namespace atma::meta::lazy
{
	template <typename F, typename... Ts>
	using cc = typename detail::_defer_<(sizeof(Ts...) < 1'000'000), F>::template apply<Ts...>;
}

// lift
export namespace atma::meta::lazy
{
	template <template <typename...> typename F, typename C = identity>
	struct lift
	{
		template <typename... Ts>
		using apply = cc<C, F<Ts>...>;
	};
}

// invoke
export namespace atma::meta::lazy
{
	template <typename F, typename... Args>
	using invoke = typename F::template apply<Args...>;
}

// delist
namespace atma::meta::lazy::detail
{
	template <typename C, typename L, typename... Ts>
	struct _delist_;

	template <typename C, typename... Ts, typename... ATs>
	struct _delist_<C, list<Ts...>, ATs...>
	{
		using type = cc<C, Ts..., ATs...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename C>
	struct delist
	{
		template <typename... Ts>
		using apply = typename detail::_delist_<C, Ts...>::type;
	};
}

// map
#if 0
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
#endif


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
			using type = typename fold_impl<F, lazy::invoke<F, I, XH>, list<Xs...>>::type;
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
		using invoke = lazy::invoke<F, Ts..., Us...>;
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

	//template <typename... Args>
	//using all_same = all<map<detail::same_op, typename detail::_all_same3_<Args...>::P>>;
}


// if
namespace atma::meta::lazy::detail
{
	template <typename>
	struct _if_;

	template <>
	struct _if_<bool_<false>>
	{
		template <typename, typename FalseType>
		using apply = FalseType;
	};

	template <>
	struct _if_<bool_<true>>
	{
		template <typename TrueType, typename>
		using apply = TrueType;
	};
}

export namespace atma::meta::lazy
{
	template <typename C = identity>
	struct if_
	{
		template <typename Cond, typename TrueValue, typename FalseValue>
		using apply = cc<C, typename detail::_if_<Cond>::template apply<TrueValue, FalseValue>>;
	};

	template <>
	struct if_<identity>
	{ // specialization for identity to do no work
		template <typename Cond, typename TrueValue, typename FalseValue>
		using apply = typename detail::_if_<Cond>::template apply<TrueValue, FalseValue>;
	};
}

export namespace atma::meta
{
	template <typename Pred, typename TrueBranch, typename FalseBranch>
	using if_ = typename lazy::if_<lazy::identity>::template apply<Pred, TrueBranch, FalseBranch>;
}

// map
namespace atma::meta::lazy
{
	template <typename F, typename C = listify>
	struct map
	{
		template <typename... Ts>
		using apply = cc<C, typename F::template apply<Ts>...>;
	};
}

export namespace atma::meta
{
	template <template <typename> typename F, typename List>
	using map = lazy::cc<lazy::delist<lazy::map<lazy::lift<F>>>, List>;
}

//
// find_if
//
namespace atma::meta::lazy::detail
{
	template <
		size_t,           // 0: empty list, 1: not empty
		typename FC,      // found continuation
		typename NFC = FC // not-found continuation
	>
	struct _find_;

	struct lmao {};

	template <typename FC, typename NFC>
	struct _find_<0, FC, NFC>
	{ // empty list
		template <typename, typename... Ts>
		using apply = invoke<NFC>;
	};

	template <typename FC, typename NFC>
	struct _find_<1, FC, NFC>
	{ // non-empty list
		template <typename F, typename E, typename... Ts>
		using apply = invoke<
			if_<>, invoke<F, E>,
				invoke<FC, E, Ts...>,
				invoke<_find_<(sizeof...(Ts) > 0), FC, NFC>, F, Ts...>>;
	};
}

export namespace atma::meta::lazy
{
	template <typename FC = listify, typename NFC = FC>
	struct find_if
	{
		template <typename F, typename... Ts>
		using apply = typename detail::_find_<(sizeof...(Ts) > 0), FC, NFC>::template apply<F, Ts...>;
	};
}

export namespace atma::meta
{
	template <typename Pred, typename List>
	using find_if = typename lazy::delist<lazy::find_if<>>::template apply<List>;
}


//template <typename T>
//struct is_signed : std::bool_constant<std::is_signed_v<T>>
//{};

struct is_signed
{
	template <typename T>
	using apply = std::bool_constant<std::is_signed_v<T>>;
};

struct make_unsigned
{
	template <typename T>
	using apply = std::make_unsigned_t<T>;
};

//template <typename T>
//using mu = std::make_unsigned_t

template <typename... Ts>
struct blahz
{};


struct blahzify
{
	template <typename... Ts>
	using apply = blahz<Ts...>;
};

template <typename C = atma::meta::listify>
struct size_is_32bit_or_less
{
	template <typename... Ts>
	using apply = atma::meta::lazy::cc<C, std::bool_constant<sizeof(Ts) <= 4>...>;
};

export void test_mythings()
{
	using namespace atma;
	using namespace atma::meta;

	static_assert(std::is_same_v<
		list<bool_<true>, bool_<true>, bool_<true>, bool_<false>>,
		typename lazy::invoke<lazy::map<make_unsigned, size_is_32bit_or_less<>>, int, short, long, long long>
	>);

	static_assert(std::is_same_v<
		list<unsigned int, unsigned short, unsigned long>,
		map<std::make_unsigned_t, list<int, short, long>>
	>);


	static_assert(std::is_same_v<lazy::invoke<lazy::map<make_unsigned>, int, short, long>, list<unsigned int, unsigned short, unsigned long>>);

	static_assert(std::is_same_v<lazy::invoke<lazy::if_<>, bool_<true>, int, float>, int>);
	static_assert(std::is_same_v<if_<bool_<true>, int, float>, int>);

	using blah = typename lazy::find_if<>::template apply<is_signed, unsigned char, unsigned int, int, unsigned long>;
	
	static_assert(std::is_same_v<blah, list<int, unsigned long>>);

	//static_assert(std::is_same_v<atma::meta::find<is_signed, unsigned char, unsigned int, int, unsigned long>, int>);
}
