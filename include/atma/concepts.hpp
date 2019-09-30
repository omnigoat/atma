#pragma once

#include <atma/meta.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/enum.hpp>

// specifies, is_true, is_false
namespace atma::concepts
{
	template <bool x>
	using is_true = std::bool_constant<x>;

	template <typename Arg, typename = std::enable_if_t<!Arg::value>>
	struct is_false : std::true_type
	{};

	namespace detail
	{
		template <typename List, typename = std::enable_if_t<meta::all<List>::value>> 
		struct specifies_
			: std::true_type
		{};
	}

	template <typename... Args>
	using specifies = detail::specifies_<meta::list<Args...>>;
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
	template <typename Concept, typename...>
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

	template <typename Concept, typename... Types>
	struct models : decltype(detail::models_<Types...>(meta::nullptr_<Concept>))
	{};

	template <typename Concept, typename... Types>
	inline constexpr bool models_v = models<Concept, Types...>::value;

	template <typename Concept, typename... Types>
	constexpr bool model_of(Types&&...)
	{
		return models<Concept, Types...>::value;
	}

	template <typename Concept, typename... Types>
	using enable_if_concept_applies_t = std::enable_if_t<models_v<Concept, Types...>>;
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


#define CONCEPT_NOT_MODELS_(concept, ...) \
    CONCEPT_NOT_MODELS_II_(ATMA_PP_CAT(_concept_models_, __COUNTER__), concept, __VA_ARGS__)

#define CONCEPT_NOT_MODELS_II_(arg_name, ...)                          \
    int arg_name = 42,                                                 \
    std::enable_if_t<                                                  \
        (arg_name == 43) || !::atma::concepts::models_v<__VA_ARGS__>,  \
        int                                                            \
    > = 0                                                              \
    /**/


#define SPECIFIES_TYPE(...) \
	decltype(std::void_t<__VA_ARGS__>(), std::true_type())

#define SPECIFIES_EXPR(...) \
    decltype(__VA_ARGS__, std::true_type())

#define SPECIFIES_EXPR_OF_TYPE(type, ...) \
	::std::is_same<type, decltype(__VA_ARGS__)>

#define SPECIFIES_EXPR_OF_TYPEISH(type, ...) \
	::std::is_convertible<decltype(__VA_ARGS__), type>


#define CONTRACT_FWDS_TO_II(s, data, elem) std::declval<elem>()

//
//  CONTRACT_CALL_TEST
//  --------------------
//    calls another function by passing each type-argument as a std::declval, so
//    that you can use function-argument-variables in SPECIFIES_EXPR, for example.
//
//    see random_iterator_concept for an example
//
#define CONTRACT_CALL_TEST(fn_name, ...) \
	decltype(fn_name( \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(CONTRACT_FWDS_TO_II, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) \
	))

//
//  CONTRACT_FWDS_TO
//  -------------------
//    calls another function by passing each type-argument as a std::declval, so
//    that you can use function-argument-variables in SPECIFIES_EXPR, for example.
//
//    as opposed to CONTRACT_CALL_TEST, this specifies the entire contract, like
//
//       template <typename It>
//       CONTRACT_FWDS_TO(test, It&, It&, typename std::iterator_traits<It>::difference_type);
//
#define CONTRACT_FWDS_TO(fn_name, ...) \
	auto contract() -> decltype(fn_name( \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(CONTRACT_FWDS_TO_II, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) \
	))


// concept: integrals
namespace atma
{
	struct integral_concept
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_integral<T>
		>;
	};

	struct signed_integral_concept
		: concepts::refines<integral_concept>
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_signed<T>
		>;
	};

	struct unsigned_integral_concept
		: concepts::refines<integral_concept>
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_unsigned<T>
		>;
	};

	static_assert(concepts::models_v<integral_concept, int>);
	static_assert(concepts::models_v<signed_integral_concept, int>);
	static_assert(concepts::models_v<unsigned_integral_concept, unsigned int>);
	static_assert(!concepts::models_v<unsigned_integral_concept, int>);
}


// concepts: conversion
namespace atma::concepts
{
	struct implicitly_convertible
	{
		template <typename From, typename To>
		auto contract() -> specifies<
			std::is_convertible<From, To>
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

// concept: copy-constructible
namespace atma
{
	struct copy_constructible_concept
	{
		template <typename T, typename U = T>
		auto contract() -> concepts::specifies
		<
			SPECIFIES_EXPR(T(std::declval<U>()))
		>;
	};
}

namespace atma
{
	struct assignable_concept
	{
		template <typename T, typename U = T>
		auto contract() -> concepts::specifies
		<
			SPECIFIES_EXPR(std::declval<T&>() = std::declval<U>())
		>;
	};
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
			same_t<Ts...>
		>;
	};
}

// concepts: iterators
namespace atma::concepts
{
	struct iterator_concept
	{
		template <typename It>
		auto contract() -> specifies
		<
			SPECIFIES_TYPE(typename std::iterator_traits<It>::value_type),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::difference_type),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::reference),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::pointer),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::iterator_category),

			SPECIFIES_EXPR(*std::declval<It>()),
			SPECIFIES_EXPR_OF_TYPE(It&, ++std::declval<It&>())
		>;
	};

	struct forward_iterator_concept
		: refines<iterator_concept>
	{
		template <typename It>
		auto contract() -> specifies<
			SPECIFIES_EXPR_OF_TYPE(It, std::declval<It&>()++),
			SPECIFIES_EXPR_OF_TYPE(typename std::iterator_traits<It>::reference, *std::declval<It&>()++)
		>;
	};

	struct bidirectional_iterator_concept
		: refines<forward_iterator_concept>
	{
		template <typename It>
		auto contract() -> specifies<
			SPECIFIES_EXPR_OF_TYPE(It&, --std::declval<It&>()),
			SPECIFIES_EXPR_OF_TYPE(It, std::declval<It&>()--),
			SPECIFIES_EXPR_OF_TYPE(typename std::iterator_traits<It>::reference, *std::declval<It&>()--)
		>;
	};

	struct random_iterator_concept
		: refines<bidirectional_iterator_concept>
	{
		template <typename It,
			typename...,
			typename difference_type = typename std::iterator_traits<It>::difference_type,
			typename reference_type = typename std::iterator_traits<It>::reference
		>
		auto contract(It& a, It& b, difference_type n) -> specifies
		<
			SPECIFIES_EXPR_OF_TYPE(It&, a += n),
			SPECIFIES_EXPR_OF_TYPE(It, a + n),
			SPECIFIES_EXPR_OF_TYPE(It, n + a),
			SPECIFIES_EXPR_OF_TYPE(It&, a -= n),
			SPECIFIES_EXPR_OF_TYPE(It, a - n),
			SPECIFIES_EXPR_OF_TYPE(difference_type, b - a),
			SPECIFIES_EXPR_OF_TYPEISH(reference_type, a[n]),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a < b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a <= b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a >= b),
			SPECIFIES_EXPR_OF_TYPEISH(bool, a > b)
		>;
	};

	struct contiguous_iterator_concept
		: refines<random_iterator_concept>
	{
		template <typename It>
		auto contract() -> specifies<
			// we can't fully express the constraint 'contiguous' - but we *can* ask
			// for std::addressof the first element plus a difference type to be dereferenceable
			SPECIFIES_EXPR(*(std::addressof(*std::declval<It>()) + std::declval<typename std::iterator_traits<It>::difference_type>()))
		>;
	};
}