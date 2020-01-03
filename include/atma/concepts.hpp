#pragma once

#include <atma/meta.hpp>
#include <atma/types.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/comma_if.hpp>


// specifies
namespace atma::concepts
{
	namespace detail
	{
		template <bool b, typename = std::enable_if_t<b>> 
		struct specifies_
			: std::true_type
		{};
	}

	template <auto... Vals>
	using specifies = detail::specifies_<(Vals && ...)>;
}


//
// true_v
// --------
//  this allows us to just make a random type equate to true,
//  which is useful in a SFINAE context where we just want to
//  see if something is possible. if it compiles -> true.
//
namespace atma::concepts::detail
{
	template <typename T>
	constexpr bool true_v = true;

	template <auto... Vs>
	constexpr bool all_true_v = (Vs && ...);
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

// models
namespace atma::concepts
{
	template <typename Concept, typename...>
	struct models;

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
	inline constexpr bool models_ref_v = models<Concept, rmref_t<Types>...>::value;

	template <typename Concept, typename... Types>
	inline constexpr bool model_of(Types&&...)
	{
		return true; //models_v<Concept, Types...>;
	}

	template <typename Concept>
	inline constexpr bool model_ofs()
	{
		return true; //models_v<Concept, Types...>;
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


#define MODELS_ARGS_M(r,d,i,x) BOOST_PP_COMMA_IF(i) ::std::remove_reference_t<decltype(x)>
#define MODELS_ARGS(conceptname, ...) \
	::atma::concepts::models_v<conceptname, BOOST_PP_SEQ_FOR_EACH_I(MODELS_ARGS_M, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>

#define MODELS_NOT_ARGS(conceptname, ...) \
	!::atma::concepts::models_v<conceptname, BOOST_PP_SEQ_FOR_EACH_I(MODELS_ARGS_M, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>


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



#define SPECIFIES_TYPE(type_expr) \
	::atma::concepts::detail::true_v<type_expr>

#define SPECIFIES_EXPR(expr) \
	::atma::concepts::detail::true_v<decltype(expr)>

#define SPECIFIES_EXPR_OF_TYPE(type, expr) \
	::std::is_same_v<type, decltype(expr)>

#define SPECIFIES_EXPR_OF_TYPEISH(type, expr) \
	::std::is_convertible_v<decltype(expr), type>

#define SPECIFIES_CONCEPT_MODELS(concept_name, ...) \
	::atma::concepts::models_v<concept_name, __VA_ARGS__>


// concept: integrals
namespace atma
{
	struct integral_concept
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_integral_v<T>
		>;
	};

	struct signed_integral_concept
		: concepts::refines<integral_concept>
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_signed_v<T>
		>;
	};

	struct unsigned_integral_concept
		: concepts::refines<integral_concept>
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			std::is_unsigned_v<T>
		>;
	};

	static_assert(concepts::models_v<integral_concept, int>);
	static_assert(concepts::models_v<signed_integral_concept, int>);
	static_assert(concepts::models_v<unsigned_integral_concept, unsigned int>);
	static_assert(!concepts::models_v<unsigned_integral_concept, int>);
}


// concepts: conversion
namespace atma
{
	struct implicitly_convertible_concept
	{
		template <typename From, typename To>
		auto contract() -> concepts::specifies<
			std::is_convertible_v<From, To>
		>;
	};

	struct explicitly_convertible_concept
	{
		template <typename From, typename To>
		auto contract(From&& from) -> concepts::specifies<
			SPECIFIES_EXPR(static_cast<To>(from))
		>;
	};

	struct convertible_concept
		: concepts::refines<implicitly_convertible_concept, explicitly_convertible_concept>
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
		auto contract(T& to, U& from) -> concepts::specifies
		<
			SPECIFIES_EXPR(to = from)
		>;
	};
}

// concept: Same
namespace atma::concepts
{
	struct Same
	{
		template <typename T, typename... Us>
		auto contract() -> specifies<
			(std::is_same_v<T, Us> && ...)
		>;
	};
}

// concepts: iterators
namespace atma
{
	struct iterator_concept
	{
		template <typename It>
		auto contract(It& a) -> concepts::specifies
		<
			SPECIFIES_TYPE(typename std::iterator_traits<It>::value_type),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::difference_type),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::reference),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::pointer),
			SPECIFIES_TYPE(typename std::iterator_traits<It>::iterator_category),

			SPECIFIES_EXPR(*std::declval<It>()),
			SPECIFIES_EXPR_OF_TYPE(It&, ++a)
		>;
	};

	struct forward_iterator_concept
		: concepts::refines<iterator_concept>
	{
		template <typename It,
			typename...,
			typename reference = typename std::iterator_traits<It>::reference>
		auto contract(It& a) -> concepts::specifies<
			SPECIFIES_EXPR_OF_TYPE(It, a++),
			SPECIFIES_EXPR_OF_TYPE(reference, *a++)
		>;
	};

	struct bidirectional_iterator_concept
		: concepts::refines<forward_iterator_concept>
	{
		template <typename It,
			typename...,
			typename reference = std::iterator_traits<It>::reference>
		auto contract(It& a) -> concepts::specifies<
			SPECIFIES_EXPR_OF_TYPE(It&, --a),
			SPECIFIES_EXPR_OF_TYPE(It, a--),
			SPECIFIES_EXPR_OF_TYPE(reference, *a--)
		>;
	};

	struct random_iterator_concept
		: concepts::refines<bidirectional_iterator_concept>
	{
		template <typename It,
			typename...,
			typename difference_type = typename std::iterator_traits<It>::difference_type,
			typename reference_type = typename std::iterator_traits<It>::reference
		>
		auto contract(It& a, It& b, difference_type n) -> concepts::specifies
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
		: concepts::refines<random_iterator_concept>
	{
		template <typename It,
			typename...,
			typename difference_type = typename std::iterator_traits<It>::difference_type>
		auto contract(It a, difference_type n) -> concepts::specifies<
			// we can't fully express the constraint 'contiguous' - but we *can* ask
			// for std::addressof the first element plus a difference type to be dereferenceable
			SPECIFIES_EXPR(*(std::addressof(*a) + n))
		>;
	};
}
