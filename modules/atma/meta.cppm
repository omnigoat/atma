
// yes, much of this file is inspired by kvasir.mpl

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


// integral types
export namespace atma::meta
{
	struct nothing {};

	template <typename...>
	using void_ = void;

	template <auto x>
	using integral_constant_of = std::integral_constant<decltype(x), x>;

	template <bool         x> using bool_    = integral_constant_of<x>;
	template <char         x> using char_    = integral_constant_of<x>;
	template <int          x> using int_     = integral_constant_of<x>;
	template <unsigned int x> using uint_    = integral_constant_of<x>;
	template <uint32_t     x> using uint32_  = integral_constant_of<x>;
	template <uint64_t     x> using uint64_  = integral_constant_of<x>;
	template <size_t       x> using usize_   = integral_constant_of<x>;
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


// any_t
export namespace atma::meta
{
	template <typename T = void>
	struct any_t
	{
		constexpr any_t() = default;

		constexpr any_t(T&&)
		{}
	};

	template <>
	struct any_t<void>
	{
		constexpr any_t() = default;

		template <typename T>
		constexpr any_t(T&&)
		{}
	};
}


// identity
export namespace atma::meta::lazy
{
	struct identity
	{
		template <typename T>
		using f = T;
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


//
// dependent call
//
namespace atma::meta::lazy::detail
{
	// dependent-call
	template <typename, bool>
	struct dcc_impl
	{};

	template <typename F>
	struct dcc_impl<F, true> : F
	{};


	// dependent-call-function
	template <bool>
	struct dccf_impl
	{
		template <template <typename...> class F, typename... Args>
		using f = F<Args...>;
	};

	template <>
	struct dccf_impl<false>
	{
		template <template <typename...> class F, typename... Args>
		using f = F<>;
	};

}

// dependent-call
// ----------------
export namespace atma::meta::lazy
{
	template <typename F, size_t Sz>
	using dcc = typename detail::dcc_impl<F, Sz < 1024>;

	template <template <typename...> typename F, typename... Args>
	using dccf = typename detail::dccf_impl<(sizeof...(Args) > 0)>::template f<F, Args...>;
}

#if 1
// call-continuation
// -------------------
// 
//  yeah requires explanation
//
export namespace atma::meta::lazy
{
	template <typename F, typename... Args>
	using cc = typename dcc<F, sizeof...(Args)>::template f<Args...>;
}
#else
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
#endif


//
// continuation-from-eager-template
// ----------------------------------
//
// Since all continuations/metafunctions in our metaprogram have
// their main operands sent through member-alias 'f', then any
// (usually eager-written) metafunction that processes all its
// template arguments as class-arguments must be converted.
// 
//    template <typename A, typename B>
//    struct less_than
//    {
//        using type = bool_<A::value < B::value>;
//    };
//
namespace atma::meta::lazy::detail
{
	template <typename F, typename C = identity>
	struct cfu_impl
	{
		template <typename... Args>
		struct fn
		{
			using f = cc<C, typename F::template f<Args...>>;
		};
	};

	template <typename F>
	struct cfu_impl<F, identity>
	{
		template <typename... Args>
		struct fn
		{
			using f = typename F::template f<Args...>;
		};
	};
}

export namespace atma::meta::lazy
{
	template <typename F, typename C = identity>
	using cfu = typename detail::cfu_impl<F, C>::fn;
}


//
// continuation-from-eager
// -------------------------
//
// Since all continuations/metafunctions in our metaprogram have
// their main operands sent through member-alias 'f', then any
// (usually eager-written) metafunction that processes all its
// template arguments as class-arguments must be converted.
// 
//    template <typename A, typename B>
//    using type = bool_<A::value < B::value>;
// 
//    cfe<less_than, Continuation>
//
export namespace atma::meta::lazy
{
	template <template <typename...> class F, typename C = identity>
	struct cfe
	{
		template <typename... Args>
		using f = typename C::template f<dccf<F, Args...>>;
	};

	template <template <typename...> class F>
	struct cfe<F, identity>
	{ // specialization for identity - no need to continue
		template <typename... Args>
		using f = dccf<F, Args...>;
	};
}

//
// continuation-from-lazy
// ------------------------
//
//    struct less_than
//    {
//        template <typename A, typename B>
//        using type = bool_<A::value < B::value>;
//    };
// 
//    non-mpl lazy function  ->  mpl-approved lazy function
// 
//    cfl<less_than, Continuation>
//
export namespace atma::meta::lazy
{
	template <template <typename...> class F, typename C = identity>
	struct cfl
	{
		template <typename... Args>
		using f = typename C::template f<typename dccf<F, Args...>::type>;
	};

	template <template <typename...> class F>
	struct cfl<F, identity>
	{ // specialization for identity - no need to continue
		template <typename... Args>
		using f = typename dccf<F, Args...>::type;
	};
}

//
// continuation-from-value
// -------------------------
//
// requires explanation
//
export namespace atma::meta::lazy
{
	template <typename T, typename C = identity>
	struct cfv
	{
		template <typename...>
		using f = typename C::template f<T>;
	};
}


// invoke
export namespace atma::meta::lazy
{
	template <typename F, typename... Args>
	using invoke = typename F::template f<Args...>;
}

//
// list
//
export namespace atma::meta
{
	template <typename... As>
	struct list
	{
		using type = list;
		static inline constexpr size_t size = sizeof...(As);
	};
}


//
// listify
//
export namespace atma::meta
{
	namespace lazy
	{
		struct listify
		{
			template <typename... Es>
			using f = list<Es...>;
		};
	}

	template <typename... Es>
	using listify = typename lazy::listify::template f<Es...>;
}

//
// unpack
//
namespace atma::meta::lazy::detail
{
	template <bool front, typename, typename, typename...>
	struct unpack_impl;

	template <typename C, typename... Es, typename... Args>
	struct unpack_impl<true, C, list<Es...>, Args...>
	{
		using f = cc<C, Es..., Args...>;
	};

	template <typename C, typename... Es, typename... Args>
	struct unpack_impl<false, C, list<Es...>, Args...>
	{
		using f = cc<C, Args..., Es...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename C>
	struct unpack_front
	{
		template <typename List, typename... Args>
		using f = typename detail::unpack_impl<true, C, List, Args...>::f;
	};

	template <typename C>
	struct unpack_back
	{
		template <typename List, typename... Args>
		using f = typename detail::unpack_impl<false, C, List, Args...>::f;
	};

	// order doesn't matter, pick one
	template <typename C>
	using unpack = unpack_back<C>;
}


//
// at
// ----
// 
// gets the argument at index N
//
namespace atma::meta::lazy::detail
{
	template <size_t N, typename C>
	struct _at_
	{
		template <typename Head, typename... Tail>
		using f = cc<_at_<N - 1, C>, Tail...>;
	};

	template <typename C>
	struct _at_<0, C>
	{
		template <typename E0, typename...>
		using f = cc<C, E0>;
	};

	template <typename C>
	struct _at_<1, C>
	{
		template <typename E0, typename E1, typename... Elements>
		using f = cc<C, E1>;
	};

	template <typename C>
	struct _at_<2, C>
	{
		template <typename E0, typename E1, typename E2, typename...>
		using f = cc<C, E2>;
	};

	template <typename C>
	struct _at_<3, C>
	{
		template <typename E0, typename E1, typename E2, typename E3, typename...>
		using f = cc<C, E3>;
	};
}

export namespace atma::meta::lazy
{
	template <typename N, typename C = identity>
	struct at
	{
		template <typename... Elements>
		using f = cc<detail::_at_<N::value, C>, Elements...>;
	};
}

export namespace atma::meta
{
	template <size_t N, typename List>
	using at = lazy::cc<lazy::unpack<lazy::at<usize_<N>>>, List>;
}







//
// drop
//
#if 0
namespace atma::meta::lazy::detail
{
	template <size_t N, typename C>
	struct _drop_
	{
		template <typename Head, typename... Tail>
		using f = cc<typename _drop_<N - 1, C>::template f<Tail...>>;
	};

	template <typename C>
	struct _drop_<0, C>
	{
		template <typename E0, typename...>
		using f = cc<C, E0>;
	};

	template <typename C>
	struct _drop_<1, C>
	{
		template <typename E0, typename E1, typename... Elements>
		using f = cc<C, E1>;
	};

	template <typename C>
	struct _drop_<2, C>
	{
		template <typename E0, typename E1, typename... Elements>
		using f = cc<C, Elements...>;
	};

	template <typename C>
	struct _drop_<3, C>
	{
		template <typename E0, typename E1, typename E2, typename... Elements>
		using f = cc<C, Elements...>;
	};
}

export namespace atma::meta::lazy
{
	template <typename Idx, typename C = identity>
	struct drop;

	template <std::integral Integral, template <Integral> typename MI, Integral Idx, typename C>
	struct drop<MI<Idx>, C>
	{
		template <typename... Elements>
		using f = typename detail::_drop_<Idx, C>::template f<Elements...>;
	};
}

export namespace atma::meta
{
	template <size_t N, typename List>
	using drop = lazy::cc<lazy::unpack<lazy::drop<usize_<N>>>, List>;
}
#endif






#if 0
export namespace atma::meta
{
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
#endif



//
// list_element_t
//
export namespace atma::meta::lazy
{
	template <size_t Idx, typename C = identity>
	struct list_element_t
	{
		template <typename Head, typename... Rest>
		using f = cc<list_element_t<Idx - 1, C>, Rest...>;
	};

	template <typename C>
	struct list_element_t<0, C>
	{
		template <typename Head, typename... Rest>
		using f = cc<C, Head>;
	};
}

export namespace atma::meta
{
	template <size_t Idx, typename List>
	using list_element_t = lazy::cc<lazy::unpack<lazy::list_element_t<Idx>>, List>;
}

//
// fold-left
// -----------
// 
//
namespace atma::meta::lazy::detail
{
	template <size_t Argc>
	struct foldl_impl
	{
		template <typename F, typename Acc, typename Arg0, typename... Args>
		using f = typename foldl_impl<sizeof...(Args)>::template f<F, typename F::template f<Acc, Arg0>, Args...>;
	};

	template <>
	struct foldl_impl<0>
	{
		template <typename, typename Acc>
		using f = Acc;
	};
}

// todo: foldl selector

export namespace atma::meta::lazy
{
	template <typename F, typename C = identity>
	struct foldl
	{
		template <typename Init, typename... Args>
		using f = cc<C, cc<detail::foldl_impl<sizeof...(Args)>, F, Init, Args...>>;
	};
}

export namespace atma::meta
{
	template <template <typename...> typename F, typename Init, typename List>
	using foldl = lazy::cc<lazy::unpack_back<lazy::foldl<lazy::cfe<F>>>, List, Init>;

	template <template <typename...> typename F, typename Init, typename... Es>
	using foldl_pack = lazy::cc<lazy::foldl<lazy::cfe<F>>, Init, Es...>;
}


//
// fold-right
// -----------
// 
//
namespace atma::meta::lazy::detail
{
	template <size_t Argc>
	struct foldr_impl
	{
		template <typename F, typename Arg0, typename... Args>
		using f = typename F::template f<Arg0, cc<foldr_impl<sizeof...(Args)>, F, Args...>>;
	};

	template <>
	struct foldr_impl<2>
	{
		template <typename F, typename Arg0, typename Arg1>
		using f = typename F::template f<Arg0, Arg1>;
	};

	template <>
	struct foldr_impl<1>
	{
		template <typename F, typename Arg0>
		using f = Arg0;
	};
}

export namespace atma::meta::lazy
{
	template <typename F, typename C = identity>
	struct foldr
	{
		template <typename... Args>
		using f = cc<C, cc<detail::foldr_impl<sizeof...(Args)>, F, Args...>>;
	};
}

namespace atma::meta::detail
{
	template <typename F, typename...>
	struct foldr_selector;

	template <typename F, typename... Es>
	struct foldr_selector<F, list<Es...>>
	{ // no initial value
		using type = lazy::cc<lazy::foldr<F>, Es...>;
	};

	template <typename F, typename... Es, typename Init>
	struct foldr_selector<F, Init, list<Es...>>
	{ // initial value
		using type = lazy::cc<lazy::foldr<F>, Es..., Init>;
	};
}

export namespace atma::meta
{
	template <template <typename...> typename F, typename... InitAndOrList>
	using foldr = typename detail::foldr_selector<lazy::cfe<F>, InitAndOrList...>::type;

	template <template <typename...> typename F, typename... Es>
	using foldr_pack = lazy::cc<lazy::foldr<lazy::cfe<F>>, Es...>;
}



// logical operators
export namespace atma::meta::lazy
{
	template <typename C = identity>
	struct and_
	{
		template <typename A, typename B>
		using f = typename C::template f<bool_<A::value && B::value>>;
	};

	template <typename C = identity>
	struct or_
	{
		template <typename A, typename B>
		using f = typename C::template f<bool_<A::value || B::value>>;
	};
}

export namespace atma::meta
{
	template <typename A, typename B>
	using and_ = bool_<A::value && B::value>;

	template <typename A, typename B>
	using or_ = bool_<A::value || B::value>;
}


// all/any
export namespace atma::meta
{
	//template <typename List>
	//using all = foldl<and_, bool_<true>, List>;
	//
	//template <typename List>
	//using any = foldl<or_, bool_<false>, List>;
}

// if
namespace atma::meta::lazy::detail
{
	template <bool, typename, typename>
	struct _if_;

	template <typename TC, typename FC>
	struct _if_<false, TC, FC> : FC
	{ };

	template <typename TC, typename FC>
	struct _if_<true, TC, FC> : TC
	{ };
}

export namespace atma::meta::lazy
{
	template <typename Predicate, typename TC, typename FC>
	struct if_
	{
		template <typename... Args>
		using f = typename detail::_if_<dcc<Predicate, sizeof...(Args)>::template f<Args...>::value, TC, FC>::template f<Args...>;
	};
}

export namespace atma::meta
{
	template <bool C>
	struct is_true
	{
		template <typename...>
		using f = bool_<C>;
	};

	template <bool Condition, typename TrueBranch, typename FalseBranch>
	using if_ = lazy::cc<lazy::if_<is_true<Condition>, lazy::at<usize_<0>>, lazy::at<usize_<1>>>, TrueBranch, FalseBranch>;
}

// map
export namespace atma::meta::lazy
{
	template <typename F, typename C = listify>
	struct map
	{
		template <typename... Ts>
		using f = typename C::template f<typename F::template f<Ts>...>;
	};
}

export namespace atma::meta
{
	template <template <typename> typename F, typename List>
	using map = typename lazy::unpack<lazy::map<lazy::cfe<F>>>::template f<List>;
}

//
// permutations lazy-stylez
//
export namespace atma::meta
{
	//  
}


//
// permutations
//
export namespace atma::meta::old
{
	template <typename L, size_t PermSize>
	struct list_perm
	{
		using list_type = L;
		static inline constexpr size_t perm_size = PermSize;

		template <size_t Idx>
		using list_element_type = list_element_t<(Idx / perm_size) % list_type::size, list_type>;
	};

	template <typename Acc, typename... Lists>
	struct generate_list_perms;

	template <typename Acc>
	struct generate_list_perms<Acc>
	{
		using type = Acc;
	};

	template <typename... Acc, typename List, typename... Rest>
	struct generate_list_perms<list<Acc...>, List, Rest...>
	{
		using element = list_perm<List, (Rest::size * ... * 1)>;
		using type = typename generate_list_perms<list<Acc..., element>, Rest...>::type;
	};

	template <typename ListPerms, typename IndexSequence>
	struct splat_perms {};

	template <size_t Idx, typename... Perms>
	struct perm_get
	{
		using type = list<typename Perms::template list_element_type<Idx>...>;
	};

	template <typename... Perms, size_t... Idxs>
	struct splat_perms<list<Perms...>, std::index_sequence<Idxs...>>
	{
		using type = list<typename perm_get<Idxs, Perms...>::type...>;
	};

	template <typename... Lists>
	struct permutations
	{
		static inline constexpr size_t total_size = (Lists::size * ...);

		using list_perms_type = typename generate_list_perms<list<>, Lists...>::type;

		using type = typename splat_perms<list_perms_type, std::make_index_sequence<total_size>>::type;
	};
}
















//template <typename T>
//struct is_signed : std::bool_constant<std::is_signed_v<T>>
//{};

struct is_signed
{
	template <typename T>
	using f = std::bool_constant<std::is_signed_v<T>>;
};

struct make_unsigned
{
	template <typename T>
	using f = std::make_unsigned_t<T>;
};

//template <typename T>
//using mu = std::make_unsigned_t

template <typename... Ts>
struct blahz
{};


struct blahzify
{
	template <typename... Ts>
	using f = blahz<Ts...>;
};

template <typename C = atma::meta::lazy::listify>
struct size_is_32bit_or_less
{
	template <typename T>
	using f = atma::meta::integral_constant_of<sizeof(T) <= 4>;
};

enum class MathsOperation
{
	Mul, Div, Add, Sub
};

template <template <typename...> typename Op, typename Value>
struct oper : Value
{};

template <typename A, typename B>
struct oper_action;

template <typename A, template <typename...> typename Op, typename Value>
struct oper_action<A, oper<Op, Value>>
{
	using type = Op<A, Value>;
};

template <typename A, template <typename...> typename Op, typename Value>
struct oper_action<oper<Op, Value>, A>
{
	using type = Op<Value, A>;
};

template <typename A, typename B>
using oper_action_t = typename oper_action<A, B>::type;

export void test_mythings()
{
	using namespace atma;
	using namespace atma::meta;


	//static_assert(std::is_same_v<
	//	list<bool_<true>, bool_<true>, bool_<true>, bool_<false>>,
	//	typename lazy::invoke<lazy::map<make_unsigned, size_is_32bit_or_less<>>, int, short, long, long long>
	//>);

	static_assert(std::is_same_v<
		list<unsigned int, unsigned short, unsigned long>,
		map<std::make_unsigned_t, list<int, short, long>>
	>);

	static_assert(std::is_same_v<
		uint_<10>,
		foldl<add, uint_<0>, list<uint_<1>, uint_<2>, uint_<3>, uint_<4>>>
	>);

	static_assert(std::is_same_v<
		uint_<33>,
		foldr<oper_action_t, uint_<3>, list<oper<sub, uint_<40>>, oper<div, uint_<21>>>>
	>);

	static_assert(std::is_same_v<
		uint_<33>,
		foldr_pack<oper_action_t, oper<sub, uint_<40>>, oper<div, uint_<21>>, uint_<3>>
	>);

	static_assert(std::is_same_v<lazy::invoke<lazy::map<make_unsigned>, int, short, long>, list<unsigned int, unsigned short, unsigned long>>);

	//static_assert(lazy::cc<lazy::if_<size_is_32bit_or_less<>,
	//	lazy::identity,
	//	lazy::identity>, int>, int>);

	//if_<false, int, float>;

	static_assert(std::is_same_v<int, if_<true, int, float>>);

	using first_param_is_signed = lazy::at<usize_<0>, lazy::cfl<std::is_signed>>;
	static_assert(std::is_same_v<int, 
		lazy::cc<lazy::if_<first_param_is_signed, lazy::at<usize_<0>>, lazy::at<usize_<1>>>, int, float>>);
	//static_assert(std::is_same_v<if_<bool_<true>, int, float>, int>);


	struct A{}; struct B{}; struct C{};

	// permutations - works
#if 1
	static_assert(std::is_same_v<typename old::permutations<list<int, float, short>, list<char, double>, list<A, B>>::type,
		list<
			list<int, char, A>,
			list<int, char, B>,
			list<int, double, A>,
			list<int, double, B>,
			list<float, char, A>,
			list<float, char, B>,
			list<float, double, A>,
			list<float, double, B>,
			list<short, char, A>,
			list<short, char, B>,
			list<short, double, A>,
			list<short, double, B>
		>
	>);
#endif
}
