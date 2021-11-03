#pragma once

#include <tuple>
#include <cstdint>

import atma.types;


// msvc does weird things with its intrinsics
#pragma warning(push)
#pragma warning(disable:4305)




namespace atma {

	namespace detail
	{
		template <template <int> class, typename> struct tuple_idxs_map_tx;

		template <uint, typename>     struct tuple_get_tx;
		template <typename, typename> struct tuple_select_tx;

		template <typename> struct tuple_head_tx;
		template <typename> struct tuple_tail_tx;

		template <template <typename...> class, typename> struct tuple_map_tx;

		template <typename> struct tuple_flip_tx;
	}


	//
	//  auto_tuple
	//  ------------
	//    an addendum to std::make_tuple and std::forward_as_tuple.
	//
	//    takes a bunch of things and creates a "storable" tuple-type. this means
	//    every rvalue-argument maps to a pure-value, and every lvalue argument
	//    stays as an lvalue.
	//
	template <typename... Types>
	inline auto auto_tuple(Types&&... types) -> std::tuple<Types...>
	{
		return std::tuple<Types...>(std::forward<Types>(types)...);
	}

	template <typename... Types>
	using auto_tuple_t = decltype(auto_tuple(std::declval<Types>()...));




	//
	//  tuple_idxs_map_t
	//  ------------------
	//    maps a function over an idxs_t and generates a tuple
	//
	//    tuple_idxs_map_t<placeholder_t, idxs_t<0, 1, 2>> ===
	//      std::tuple<placeholder_t<0>, placeholder_t<1>, placeholder_t<2>>
	//
	template <template <int> class f, typename idxs>
	using tuple_idxs_map_t = typename detail::tuple_idxs_map_tx<f, idxs>::type;

	namespace detail
	{
		template <template <int> class f, int... idxs>
		struct tuple_idxs_map_tx<f, idxs_t<idxs...>>
		{
			using type = std::tuple<typename f<idxs>::type...>;
		};
	}




	//
	//  tuple_get
	//  -----------
	//    gets an element from a tuple
	//
	//      tuple_get_t<1, std::tuple<int, float, string, dragon>> === float
	//
	//    (just hand-off to std::get<>)
	//
	template <uint idx, typename xs_t>
	using tuple_get_t = typename std::tuple_element<idx, xs_t>::type;

	template <uint idx, typename xs_t>
	inline auto tuple_get(xs_t&& xs) -> tuple_get_t<idx, xs_t>
	{
		return std::get<idx>(std::forward<xs_t>(xs));
	}




	//
	//  tuple_select
	//  ---------------
	//    tuple_select_t<idxs_t<1, 3>, std::tuple<int, float, string, dragon>>
	//      === std::tuple<float, dragon>
	//
	//    tuple_select<ids_t<1>>(std::tuple<float, int>()) == std::tuple<int>()
	//
	namespace detail
	{
		template <size_t... idxs, typename ys>
		struct tuple_select_tx<idxs_t<idxs...>, ys>
		{
			using type = std::tuple<typename std::tuple_element<idxs, ys>::type...>;

			template <typename xs_t>
			static auto call(xs_t&& xs) -> type
			{
				return std::forward_as_tuple(std::get<idxs>(std::forward<xs_t>(xs))...);
			}
		};
	}

	template <typename idxs, typename xs_t>
	using tuple_select_t = typename detail::tuple_select_tx<idxs, std::decay_t<xs_t>>::type;

	template <typename idxs, typename xs_t>
	inline auto tuple_select(xs_t&& xs) -> tuple_select_t<idxs, xs_t>
	{
		return detail::tuple_select_tx<idxs, std::decay_t<xs_t>>::call(std::forward<xs_t>(xs));
	}




	//
	//  tuple_head_t
	//  --------------
	//    returns the first type in a tuple
	//
	namespace detail
	{
		template <typename x, typename... xs>
		struct tuple_head_tx<std::tuple<x, xs...>>
		{
			using type = x;
		};
	}

	template <typename xs>
	using tuple_head_t = typename detail::tuple_head_tx<typename std::decay<xs>::type>::type;

	template <typename xs_t>
	inline auto tuple_head(xs_t&& xs) -> tuple_head_t<xs_t>
	{
		return std::get<0>(std::forward<xs_t>(xs));
	}




	//
	//  tuple_tail_t
	//  --------------
	//    returns a tuple of the trailing types of a tuple:
	//
	//      tuple_tail_t<std::tuple<int, float, string>> === std::tuple<float, string>
	//      tuple_tail_t<std::tuple<>> === SO MUCH ERROR
	//
	namespace detail
	{
		template <typename... xs>
		struct tuple_tail_tx<std::tuple<xs...>>
		{
			static_assert(sizeof...(xs) != 0, "error: tuple_tail called upon empty tuple");
			static_assert(sizeof...(xs) == 0, "error: tuple_tail incorrectly instantiated");
		};

		template <typename x, typename... xs>
		struct tuple_tail_tx<std::tuple<x, xs...>>
		{
			using type = std::tuple<xs...>;
		};
	}

	template <typename xs>
	using tuple_tail_t = typename detail::tuple_tail_tx<typename std::decay<xs>::type>::type;

	template <typename xs_t>
	inline auto tuple_tail(xs_t&& xs) -> tuple_tail_t<xs_t>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<xs_t>>::value;

		return tuple_select<idxs_range_t<1, tuple_size>>(std::forward<xs_t>(xs));
	}




	//
	//  tuple_push_back_t
	//  -------------------
	//    tuple_push_back_t<std::tuple<int, char>, double> === std::tuple<int, char, double>
	//
	namespace detail
	{
		template <typename, typename> struct tuple_push_back_tx;

		template <typename x, typename... xs>
		struct tuple_push_back_tx<std::tuple<xs...>, x>
		{
			using type = std::tuple<xs..., x>;
		};
	}

	template <typename Tuple, typename X>
	using tuple_push_back_t = typename detail::tuple_push_back_tx<Tuple, X>::type;

	template <typename xs_t, typename x_t>
	inline auto tuple_push_back(xs_t&& xs, x_t&& x) -> tuple_push_back_t<std::decay_t<xs_t>, std::decay_t<x_t>>
	{
		return std::tuple_cat(std::forward<xs_t>(xs), std::forward_as_tuple(std::forward<x_t>(x)));
	}




	//
	//  tuple_pop_back_t
	//  ------------------
	//
	namespace detail
	{
		template <typename>
		struct tuple_pop_back_tx;

		template <typename... xs>
		struct tuple_pop_back_tx<std::tuple<xs...>>
		{
			using type = tuple_select_t<idxs_range_t<0, sizeof...(xs) - 1>, std::tuple<xs...>>;
		};
	}

	template <typename Tuple>
	using tuple_pop_back_t = typename detail::tuple_pop_back_tx<Tuple>::type;




	//
	//  tuple_push_front_t
	//  -------------------
	//    tuple_push_front_t<std::tuple<int, char>, double> === std::tuple<double, int, char>
	//
	namespace detail
	{
		template <typename, typename> struct tuple_push_front_tx;

		template <typename x, typename... xs>
		struct tuple_push_front_tx<std::tuple<xs...>, x>
		{
			using type = std::tuple<x, xs...>;
		};
	}

	template <typename xs, typename x>
	using tuple_push_front_t = typename detail::tuple_push_front_tx<xs, x>::type;

	template <typename xs_t, typename x_t>
	inline auto tuple_push_front(xs_t&& xs, x_t&& x) -> tuple_push_front_t<std::decay_t<xs_t>, x_t>
	{
		return std::tuple_cat(std::forward_as_tuple(x), xs);
	}


	//
	//  tuple_pop_front_t
	//  -------------------
	//    synonymous with tuple_tail_t
	//
	template <typename Tuple>
	using tuple_pop_front_t = tuple_tail_t<Tuple>;


	//
	//  tuple_fold_t
	//  -------------
	//    I know of no earthly reason why you'd want this.
	//
	namespace detail
	{
		template <template <typename, typename> class F, typename Y, typename XS>
		struct tuple_fold_tx
		{
			using type =
				typename tuple_fold_tx<F, typename F<Y, tuple_head_t<XS>>, tuple_tail_t<XS>>::type;
		};

		template <template <typename, typename> class F, typename Y>
		struct tuple_fold_tx<F, Y, std::tuple<>>
		{
			using type = Y;
		};
	}

	template <template <typename, typename> class F, typename Initial, typename Tuple>
	using tuple_fold_t = typename detail::tuple_fold_tx<F, Initial, Tuple>::type;




	//
	//  tuple_map_t
	//  --------------------
	//    maps a constructor-type over a tuple-of-types, generating a new tuple:
	//
	//      tuple_map_t<std::add_reference, std::tuple<int, double>> === std::tuple<int&, double&>
	//      tuple_map_t<std::remove_pointer, std::tuple<int*, double>> === std::tuple<int, double>
	//
	namespace detail
	{
		template <template <typename...> class f, typename xs>
		struct tuple_map_tx;

		template <template <typename...> class f, typename... xs>
		struct tuple_map_tx<f, std::tuple<xs...>>
		{
			using type = std::tuple<typename f<xs>::type...>;
		};
	}

	template <template <typename> class f, typename xs> using tuple_map_t = typename detail::tuple_map_tx<f, xs>::type;




	//
	//  tuple_flip_t
	//  --------------
	//    flips the arguments in a tuple:
	//
	//      tuple_flip_t<std::tuple<A, B, C>> === std::tuple<C, B, A>
	//      tuple_flip_t<std::tuple<A>>       === std::tuple<A>
	//      tuple_flip_t<std::tuple<>>        === std::tuple<>
	//
	namespace detail
	{
		template <typename Tuple>
		struct tuple_flip_tx
		{
			using type = tuple_select_t<idxs_range_t<std::tuple_size<Tuple>::value - 1, -1, -1>, Tuple>;
		};
	}

	template <typename Tuple> using tuple_flip_t = typename detail::tuple_flip_tx<Tuple>::type;




	//
	//  tuple_apply/tuple_binary_apply
	//  --------------------------------
	//    takes a functor and applies it to a tuple, returning a tuple with the results
	//
	//      tuple_apply_t<is_four_t, std::tuple<int, int, int>> === std::tuple<bool, bool, bool>
	//
	//      tuple_apply(is_four_t(), make_tuple(3, 4, 5)) === make_tuple(false, true, false)
	//
	//      tuple_apply_binary_t<eq_t, std::tuple<int, int>, std::tuple<int, int>> === std::tuple<bool, bool>
	//
	//      tuple_apply_binary(eq_t(), {4, 5}, {2, 5}) === {false, true}
	//
	namespace detail
	{
		template <typename FN, typename Tuple, size_t... idxs>
		auto tuple_apply_impl(FN&& fn, Tuple&& tuple, idxs_t<idxs...>)
		{
			return auto_tuple(std::invoke(std::forward<FN>(fn), std::get<idxs>(tuple))...);
		}

		template <typename FN, typename LHS, typename RHS, size_t... idxs>
		auto tuple_binary_apply_impl(FN&& fn, LHS&& lhs, RHS&& rhs, idxs_t<idxs...>)
		{
			return auto_tuple(std::invoke(std::forward<FN>(fn), std::get<idxs>(lhs), std::get<idxs>(rhs))...);
		}
	}

	template <typename FN, typename Tuple>
	using tuple_apply_t = decltype(detail::tuple_apply_impl(std::declval<FN>(), std::declval<Tuple>(), idxs_list_t<std::tuple_size<std::remove_reference_t<Tuple>>::value>()));

	template <typename FN, typename LHS, typename RHS>
	using tuple_binary_apply_t = decltype(detail::tuple_binary_apply_impl<FN>(
		std::declval<FN>(), std::declval<LHS>(), std::declval<RHS>(),
		idxs_list_t<std::tuple_size<std::remove_reference_t<LHS>>::value>()));

	template <typename FN, typename Tuple>
	auto tuple_apply(FN&& fn, Tuple&& tuple)
	{
		return detail::tuple_apply_impl(std::forward<FN>(fn), tuple, idxs_list_t<std::tuple_size<std::remove_reference_t<Tuple>>::value>());
	}

	template <typename FN, typename LHS, typename RHS>
	auto tuple_binary_apply(FN&& fn, LHS&& lhs, RHS&& rhs)
	{
		auto const lhs_size = std::tuple_size_v<std::remove_reference_t<LHS>>;
		auto const rhs_size = std::tuple_size_v<std::remove_reference_t<RHS>>;
		static_assert(lhs_size == rhs_size, "sizes must be the same");

		return detail::tuple_binary_apply_impl(std::forward<FN>(fn), lhs, rhs, idxs_list_t<lhs_size>());
	}




	struct begin_functor_t
	{
		template <typename T>
		decltype(auto) operator ()(T&& t) const {
			return std::begin(std::forward<T>(t));
		}
	};

	struct end_functor_t
	{
		template <typename T>
		decltype(auto) operator ()(T&& t) const {
			return std::end(std::forward<T>(t));
		}
	};

	struct increment_functor_t
	{
		template <typename T>
		decltype(auto) operator ()(T&& t) const {
			return ++std::forward<T>(t);
		}
	};

	struct eq_functor_t
	{
		template <typename LHS, typename RHS>
		decltype(auto) operator ()(LHS&& lhs, RHS&& rhs) const {
			return std::forward<LHS>(lhs) == std::forward<RHS>(rhs);
		}
	};

	struct neq_functor_t
	{
		template <typename LHS, typename RHS>
		decltype(auto) operator ()(LHS&& lhs, RHS&& rhs) const {
			return std::forward<LHS>(lhs) != std::forward<RHS>(rhs);
		}
	};

	struct assign_functor_t
	{
		template <typename LHS, typename RHS>
		decltype(auto) operator ()(LHS&& lhs, RHS&& rhs) const {
			return std::forward<LHS>(lhs) = std::forward<RHS>(rhs);
		}
	};

	struct dereference_functor_t
	{
		template <typename T>
		decltype(auto) operator ()(T&& t) const {
			return *std::forward<T>(t);
		}
	};



	//
	//
	//
	namespace detail
	{
		template <typename Tuple, size_t... idxs>
		inline auto tuple_any_of_impl(Tuple&& tuple, std::index_sequence<idxs...>) -> bool
		{
			return (std::get<idxs>(tuple) || ...);
		}

		template <typename Tuple, size_t... idxs>
		inline auto tuple_all_of_impl(Tuple&& tuple, std::index_sequence<idxs...>) -> bool
		{
			return (std::get<idxs>(tuple) && ...);
		}
	}
	
	template <typename Tuple>
	inline auto tuple_any_of(Tuple&& tuple) -> bool
	{
		return detail::tuple_any_of_impl(tuple, std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
	}

	template <typename Tuple>
	inline auto tuple_all_of(Tuple&& tuple) -> bool
	{
		return detail::tuple_all_of_impl(tuple, std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
	}


	template <typename LHS, typename RHS>
	auto tuple_all_elem_eq(LHS&& lhs, RHS&& rhs) -> bool
	{
		auto const lhs_size = std::tuple_size<std::decay_t<LHS>>::value;
		auto const rhs_size = std::tuple_size<std::decay_t<RHS>>::value;
		static_assert(lhs_size == rhs_size, "sizes must be the same");

		auto vs = tuple_binary_apply(eq_functor_t(), lhs, rhs);
		return tuple_all_of(vs);
	}

	template <typename LHS, typename RHS>
	auto tuple_any_elem_eq(LHS&& lhs, RHS&& rhs) -> bool
	{
		auto const lhs_size = std::tuple_size<std::decay_t<LHS>>::value;
		auto const rhs_size = std::tuple_size<std::decay_t<RHS>>::value;
		static_assert(lhs_size == rhs_size, "sizes must be the same");

		auto vs = tuple_binary_apply(eq_functor_t(), lhs, rhs);
		return tuple_any_of(vs);
	}

	template <typename LHS, typename RHS>
	auto tuple_any_elem_neq(LHS&& lhs, RHS&& rhs) -> bool
	{
		auto const lhs_size = std::tuple_size<std::decay_t<LHS>>::value;
		auto const rhs_size = std::tuple_size<std::decay_t<RHS>>::value;
		static_assert(lhs_size == rhs_size, "sizes must be the same");

		auto vs = tuple_binary_apply(neq_functor_t(), lhs, rhs);
		return tuple_any_of(vs);
	}
}

#pragma warning(pop)
