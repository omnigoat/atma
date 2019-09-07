#pragma once

#include <atma/meta.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/enum.hpp>

// specifies, is_true, is_false
namespace atma::concepts
{
	namespace detail
	{
		template <typename Arg>
		constexpr auto is_true_() ->
			std::enable_if_t<Arg::value, std::true_type>;

		template <typename Arg>
		constexpr auto is_false_() ->
			std::enable_if_t<!Arg::value, std::true_type>;

		template <typename... Args>
		constexpr void specifies_(Args&&...);

		template <typename T = std::void_t<>>
		struct can_ : std::false_type
		{};
	}

	template <typename... Args>
	struct specifies
	{
		using type = decltype(detail::specifies_(Args{}...));
		constexpr static auto value = true;
	};

	template <typename Arg, typename = std::enable_if_t<Arg::value, std::true_type>>
	struct is_true : std::true_type
	{};

	template <typename Arg, typename = std::enable_if_t<!Arg::value, std::true_type>>
	struct is_false : std::true_type
	{};

	template <typename Arg>
	struct can : detail::can_<std::void_t<Arg>>
	{};

	template <auto Expr>
	struct valid_expr : std::true_type {};
}


// base_concept_t
namespace atma::concepts::detail
{
	template <typename Concept>
	struct base_concept {
		using type = Concept; };

	template <typename Concept, typename... Args>
	struct base_concept<Concept(Args...)> {
		using type = Concept; };

	template <typename Concept>
	using base_concept_t = typename base_concept<Concept>::type;
}

// base_concepts_t
namespace atma::concepts::detail
{
	template <typename T, typename = std::void_t<>>
	struct base_concepts {
		using type = meta::list<>; };

	template <typename T>
	struct base_concepts<T, std::void_t<typename T::base_concepts_t>> {
		using type = typename T::base_concepts_t; };

	template <typename T>
	using base_concepts_t = typename base_concepts<T>::type;
}

namespace atma::concepts
{
	template <typename Concept, typename... Ts>
	struct models;
}

// models
namespace atma::concepts
{
	namespace detail
	{
		template <typename...>
		auto models_(meta::any_t) ->
			std::false_type;
		
		template <typename... Types, typename Concept, typename = decltype(&Concept::template contract<Types...>)>
		auto models_(Concept*) ->
			meta::all<
				meta::map<
					meta::bind_back<meta::invokify<models>, Types...>,
					detail::base_concepts_t<Concept>>>;
	}

	template <typename Concept, typename... Ts>
	struct models
		: meta::bool_<decltype(detail::models_<Ts...>(meta::nullptr_<Concept>))::type::value>
	{};

	template <typename Concept, typename... Ts>
	inline constexpr bool models_v = models<Concept, Ts...>::value;

	template<typename Concept, typename... Ts>
	auto model_of(Ts&&...) ->
		std::enable_if_t<models<Concept, Ts...>::value>;
}

// refines
namespace atma::concepts
{
	template<typename... Concepts>
	struct refines
		: virtual detail::base_concept_t<Concepts>...
	{
		// definitely no vtable
		refines() = delete;

		using base_concepts_t = meta::list<Concepts...>;

		template<typename...Ts>
		void contract();
	};
}


//
//  CONCEPT_REQUIRES(...)
//  ------------------------
//    takes a list of compile-time boolean expressions that indicate whether something
//    should SFINAE itself out of existance. is essentially a template <> definition:
//
//      CONCEPT_REQUIRES(sizeof(size_t) == 4)
//      int do_sizet_thing(size_t) { ... }
//
//      CONCEPT_REQUIRES(sizeof(size_t) == 8)
//      int do_sizet_thing(size_t) { ... }
//
//    if you've already got some template arguments, use CONCEPT_REQUIRES_
//    (note the underscore).
//
#define CONCEPT_REQUIRES(...) \
    template <                                                   \
        CONCEPT_REQUIRES_(__VA_ARGS__)                           \
    >                                                            \
    /**/


//
//  CONCEPT_REQUIRES_(...)
//  -------------------------
//    takes a list of compile-time boolean expressions that indicate whether something
//    should SFINAE itself out of existance. must be used inside a template definition:
//
//      template <typename R, CONCEPT_REQUIRES_(is_range_v<R>)>
//      auto operator | (int x, R const& range) -> ...
//
#define CONCEPT_REQUIRES_(...) \
    CONCEPT_REQUIRES_II_(ATMA_PP_CAT(_CONCEPT_REQUIRES_, __COUNTER__), __VA_ARGS__)

#define CONCEPT_REQUIRES_II_(arg_name, ...)                       \
    int arg_name = 42,                                            \
    std::enable_if_t<                                             \
        (arg_name == 43) || CONCEPT_REQUIRES_LIST(__VA_ARGS__),   \
        int                                                       \
    > = 0                                                         \
    /**/


#define CONCEPT_REQUIRES_LIST_M(r,d,i,x) \
	BOOST_PP_IF(i, &&,) x

#define CONCEPT_REQUIRES_LIST(...) \
	(BOOST_PP_SEQ_FOR_EACH_I(CONCEPT_REQUIRES_LIST_M, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))


//
//  CONCEPT_MODELS_(...)
//  -------------------------
//    example:
//      template <typename Range, typename Offset,
//        CONCEPT_MODELS_(addable, Range, Offset),
//        CONCEPT_MODELS_(integral, Offset)>
//      auto operator + (R const& range, Offset x) -> ...
//
#define CONCEPT_MODELS_(concept, ...) \
    CONCEPT_MODELS_II_(ATMA_PP_CAT(_concept_models_, __COUNTER__), concept, __VA_ARGS__)

#define CONCEPT_MODELS_II_(arg_name, ...)                              \
    int arg_name = 42,                                                 \
    std::enable_if_t<                                                  \
        (arg_name == 43) || ::atma::concepts::models_v<__VA_ARGS__>,   \
        int                                                            \
    > = 0                                                              \
    /**/

#define SPECIFIES_TYPE(...) \
	decltype(std::void_t<__VA_ARGS__>(), std::true_type())

#define SPECIFIES_EXPR(...) \
    ::atma::concepts::can<decltype(__VA_ARGS__)>

#define SPECIFIES_EXPR_OF_TYPE(type, ...) \
	::atma::concepts::is_true<::std::is_same<type, decltype(__VA_ARGS__)>>

#define SPECIFIES_EXPR_OF_TYPEISH(type, ...) \
	::atma::concepts::is_true<::std::is_convertible<decltype(__VA_ARGS__), type>>


#define CONTRACT_FWDS_TO_II(s, data, elem) std::declval<elem>()

#define CONTRACT_FWDS_TO(fn_name, ...) \
	auto contract() -> decltype(fn_name( \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(CONTRACT_FWDS_TO_II, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) \
	));








// concept: integrals
namespace atma::concepts
{
	struct integral
	{
		template <typename T>
		auto contract()->specifies<
			is_true<std::is_integral<T>>
		>;
	};

	struct signed_integral
		: refines<integral>
	{
		template <typename T>
		auto contract()->specifies<
			is_true<std::is_signed<T>>
		>;
	};

	struct unsigned_integral
		: refines<integral>
	{
		template <typename T>
		auto contract()->specifies<
			is_false<std::is_unsigned<T>>
		>;
	};
}

// concepts: conversion
namespace atma::concepts
{
	struct implicitly_convertible
	{
		template <typename From, typename To>
		auto contract()->specifies<
			is_true<std::is_convertible<From, To>>
		>;
	};

	struct explicitly_convertible
	{
		template <typename From, typename To>
		auto contract(From (&from)())->specifies<
			decltype(((void) static_cast<To>(from()), 42))
		>;
	};

	struct convertible
		: refines<implicitly_convertible, explicitly_convertible>
	{};
}

// concept: Same
namespace atma::concepts
{
	struct Same
	{
		template <typename... Ts>
		struct same : std::true_type {};

		template <typename T, typename... Us>
		struct same<T, Us...> : meta::all<meta::list<meta::bool_<std::is_same<T, Us>::value>...>> {};

		template <typename... Ts>
		using same_t = typename same<Ts...>::type;

		template <typename... Ts>
		auto contract()->specifies<
			is_true<same_t<Ts...>>
		>;
	};
}

// concepts: iterators
namespace atma::concepts
{
	struct iterator_concept
	{
		template <typename Iter>
		auto contract() -> specifies
		<
			// iterator-traits
			SPECIFIES_TYPE(typename std::iterator_traits<Iter>::value_type),
			SPECIFIES_TYPE(typename std::iterator_traits<Iter>::difference_type),
			SPECIFIES_TYPE(typename std::iterator_traits<Iter>::reference),
			SPECIFIES_TYPE(typename std::iterator_traits<Iter>::pointer),
			SPECIFIES_TYPE(typename std::iterator_traits<Iter>::iterator_category),

			// basic expressions for iterator
			SPECIFIES_EXPR(*std::declval<Iter>()),
			SPECIFIES_EXPR_OF_TYPE(Iter&, ++std::declval<Iter>())
		>;
	};

	struct forward_iterator_concept
		: refines<iterator_concept>
	{
		template <typename Iter>
		auto contract() -> specifies<
			is_true<std::is_base_of<std::forward_iterator_tag, typename Iter::iterator_category>>,
			SPECIFIES_EXPR_OF_TYPE(Iter, std::declval<Iter>()++),
			SPECIFIES_EXPR_OF_TYPE(typename std::iterator_traits<Iter>::reference, *std::declval<Iter>()++)
		>;
	};


	struct bidirectional_iterator_concept
		: refines<forward_iterator_concept>
	{
		template <typename Iter>
		auto contract() -> specifies<
			SPECIFIES_EXPR_OF_TYPE(Iter&, --std::declval<Iter>()),
			SPECIFIES_EXPR_OF_TYPE(Iter, std::declval<Iter>()--),
			SPECIFIES_EXPR_OF_TYPE(typename std::iterator_traits<Iter>::reference, *std::declval<Iter>()--)
		>;
	};


	struct random_iterator_concept
		: refines<bidirectional_iterator_concept>
	{
		template <typename It>
		CONTRACT_FWDS_TO(test,
			It&, It&,
			typename std::iterator_traits<It>::difference_type,
			typename std::iterator_traits<It>::reference);

		template <typename It, typename difference_type, typename reference>
		auto test(It& a, It& b, difference_type n, reference) -> specifies
		<
			SPECIFIES_EXPR_OF_TYPE(It&, a += n),
			SPECIFIES_EXPR_OF_TYPE(It, a + n),
			SPECIFIES_EXPR_OF_TYPE(It, n + a),
			SPECIFIES_EXPR_OF_TYPE(It&, a -= n),
			SPECIFIES_EXPR_OF_TYPE(It, a - n),
			SPECIFIES_EXPR_OF_TYPE(difference_type, b - a),
			SPECIFIES_EXPR_OF_TYPEISH(reference, a[n]),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a < b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a <= b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a >= b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a > b)
		>;
	};

	struct contiguous_iterator_concept
		: refines<random_iterator_concept>
	{
		template <typename Iter>
		auto contract() -> specifies<
			// SERIOUSLY, make this std::contiguous_access_iterator_tag when available
			is_true<std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<Iter>::iterator_category>>,
			SPECIFIES_EXPR(std::declval<Iter>() - std::declval<Iter>())
		>;
	};
}