#pragma once

#include <atma/meta.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>


// contract, is_true, is_false
namespace atma::concepts
{
#if 0

#if 1
	constexpr struct contract_t
	{
		template<typename ...T>
		void operator()(T &&...) const;
	} contract2{};

	template <typename... Args>
	struct contract : decltype(contract2(Args...))
	{};

	namespace detail
	{
		template <bool> struct is_true_ii;
		template <> struct is_true_ii<true> {};
	}

	template <typename Arg>
	using is_true = detail::is_true_ii<Arg::type::value>;
#else
	template <typename... Args>
	constexpr void contract(Args&&...);

	template <typename Bool_>
	constexpr auto is_true(Bool_) ->
		std::enable_if_t<Bool_::value, int>;

	template <typename Bool_>
	constexpr auto is_false(Bool_) ->
		std::enable_if_t<!Bool_::value, int>;
#endif

#else
	
	namespace detail
	{
		template <typename Arg>
		constexpr auto is_true_(Arg) ->
			std::enable_if_t<Arg::value, std::true_type>;

		template <typename Arg>
		constexpr auto is_false_(Arg) ->
			std::enable_if_t<!Arg::value, std::true_type>;

		template <typename... Args>
		constexpr void contract_(Args&&...);
	}

	template <typename... Args>
	struct contract
	{
		using type = decltype(detail::contract_(Args{}...));
		constexpr static auto value = true;
	};


	template <typename Arg, typename = decltype(detail::is_true_(Arg{}))>
	struct is_true : std::true_type {};

	template <typename Arg, typename = decltype(detail::is_false_(Arg{}))>
	struct is_false : std::true_type {};

	
	constexpr struct is_true_t
	{
		template<typename Bool_>
		auto operator()(Bool_) const ->
			std::enable_if_t<Bool_::value, int>;
	} sis_true{};

	constexpr struct is_false_t
	{
		template<typename Bool_>
		auto operator()(Bool_) const ->
			std::enable_if_t<!Bool_::value, int>;
	} is_false{};
#endif
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
		auto models_(meta::any) ->
			std::false_type;
		
		template <typename... Ts, typename Concept, typename = decltype(&Concept::template contract<Ts...>)>
		auto models_(Concept*) ->
			meta::fold<
				meta::and_op,
				meta::bool_<true>,
				meta::map<
					meta::bind_back<meta::invokify<concepts::models>, Ts...>,
					detail::base_concepts_t<Concept>>>;
	}

	template <typename Concept, typename... Ts>
	struct models
		: meta::bool_<decltype(detail::models_<Ts...>(meta::nullptr_<Concept>))::type::value>
	{};

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

// concept: Integrals
namespace atma::concepts
{
	struct Integral
	{
		template <typename T>
		auto contract()  -> decltype(
			concepts::contract(
				concepts::is_true(std::is_signed<T>{})
			));
	};

	struct SignedIntegral
		: refines<Integral>
	{
		template <typename T>
		auto contract() -> decltype(
			concepts::contract(
				concepts::is_true(std::is_signed<T>{})
			));
	};

	struct UnsignedIntegral
		: refines<Integral>
	{
		template <typename T>
		auto contract() -> decltype(
			concepts::contract(
				concepts::is_false(std::is_signed<T>{})
			));
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
		struct same<T, Us...> : meta::all<meta::list<meta::bool_<std::is_same<T, Us>::value>...>>{};

		template<typename ...Ts>
		using same_t = typename same<Ts...>::type;

		template<typename ...Ts>
		auto contract() -> contract<
			is_true<same_t<Ts...>>
		>;
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

