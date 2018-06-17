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
		any_() = default;
		any_(T&&)
		{}
	};

	template <>
	struct any_<void>
	{
		any_() = default;
		template <typename T>
		any_(T&&)
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
namespace atma::concepts::detail
{
	template <typename...>
	auto models_(meta::any) ->
		std::false_type;

	template <typename Concept, typename... Ts, typename = decltype(&Concept::template requires_<Ts...>)>
	auto models_() ->
		meta::fold<
			meta::and_op,
			meta::map<
				meta::bind_back<meta::invokify<concepts::models>, Ts...>,
				detail::base_concepts_t<Concept>>>;
}

namespace atma::concepts
{
	template <typename Concept, typename... Ts>
	struct models
		: meta::bool_<decltype(detail::models_<Concept, Ts...>())::type::value>
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
		void requires_();
	};
}

// concept: Integrals
namespace atma::concepts
{
	struct Integral
	{
		template <typename T>
		auto requires_() -> decltype(
			concepts::valid_expr(
				concepts::is_true(std::is_integral<T>{})
			));
	};

	struct SignedIntegral
		: refines<Integral>
	{
		template <typename T>
		auto requires_() -> decltype(
			concepts::valid_expr(
				concepts::is_true(std::is_signed<T>{})
			));
	};

	struct UnsignedIntegral
		: refines<Integral>
	{
		template <typename T>
		auto requires_() -> decltype(
			concepts::valid_expr(
				concepts::is_false(std::is_signed<T>{})
			));
	};
}

// concept: Same
namespace atma::concepts
{
	constexpr struct valid_expr_t
	{
		template<typename ...T>
		void operator()(T &&...) const;
	} valid_expr;

	constexpr struct is_true_t
	{
		template<typename Bool_>
		auto operator()(Bool_) const ->
			std::enable_if_t<Bool_::value>;
	} is_true;

	constexpr struct is_false_t
	{
		template<typename Bool_>
		auto operator()(Bool_) const ->
			std::enable_if_t<Bool_::value>;
	} is_false;

	struct Same
	{
		template <typename... Ts>
		struct same : std::true_type {};
		
		template <typename T, typename... Us>
		struct same<T, Us...> : meta::fold<meta::and_op, meta::list<meta::bool_<std::is_same<T, Us>::value>...>> {};

		template<typename ...Ts>
		using same_t = typename same<Ts...>::type;

		template <typename... Ts>
		auto requires_() -> decltype(
			concepts::valid_expr(
				concepts::is_true(same_t<Ts...>{})
			));
	};

	template <typename... Ts>
	using SameC = Same::same_t<Ts...>; // This handles void better than using the Same concept
}





