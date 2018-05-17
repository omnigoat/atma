#pragma once

#include <atma/function_traits.hpp>
#include <atma/enable_if.hpp>

#include <utility>

//
//  compose_impl
//  --------------
//    creates a functor that calls one function, then passes that result to another.
//    if possible, it is constructed with a concrete functor-operator. this
//    occurs when G has a concrete functor-operator. otherwise, a templated
//    functor-operator is used.
//
namespace atma::detail
{
	template <typename>
	struct composer_ii_x;

	template <typename... Args>
	struct composer_ii_x<std::tuple<Args...>>
	{
		template <typename F, typename G>
		static decltype(auto) go(F&& f, G&& g)
		{
			return [f = std::forward<F>(f), g = std::forward<G>(g)](Args... args) { return f(g(args...)); };
		}
	};

	struct composer_x
	{
		template <typename F, typename G>
		static decltype(auto) go(F&& f, G&& g)
		{
			return composer_ii_x<typename function_traits<std::decay_t<G>>::tupled_args_type>::go(std::forward<F>(f), std::forward<G>(g));
		}
	};

	template <typename F, typename G>
	inline decltype(auto) compose_impl_concrete(F&& f, G&& g)
	{
		return composer_x::go(std::forward<F>(f), std::forward<G>(g));
	}

	template <typename F, typename G>
	inline decltype(auto) compose_impl_abstract(F&& f, G&& g)
	{
		return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&&... args) { return f(g(std::forward<decltype(args)>(args)...)); };
	}
}

namespace atma
{
	//
	// function_composition_override
	// -------------------------------
	//   allows users to override how function composition between two functions
	//   is implemented
	//
	template <typename F, typename G>
	struct function_composition_override
	{
		template <typename FF, typename GG>
		static decltype(auto) compose(FF&& f, GG&& g)
		{
			if constexpr (is_callable_v<std::remove_reference_t<GG>>)
			{
				return detail::compose_impl_concrete(std::forward<F>(f), std::forward<G>(g));
			}
			else
			{
				return detail::compose_impl_abstract(std::forward<F>(f), std::forward<G>(g));
			}
		}
	};


	//
	// compose
	// ---------
	//   takes two callable things and composes them
	//
	template <typename F, typename G>
	inline decltype(auto) compose(F&& f, G&& g) {
		return function_composition_override<std::remove_reference_t<F> , std::remove_reference_t<G>>
			::compose(std::forward<F>(f), std::forward<G>(g));
	}


	//
	// composition, like the dot-operator in Haskell, if that floats your boat
	//
	template <typename F, typename G>
	inline decltype(auto) operator % (F&& f, G&& g)
	{
		//static_assert(is_callable_v<std::remove_reference_t<F>> && is_callable_v<std::remove_reference_t<G>>, "bad callables");
		return compose(std::forward<F>(f), std::forward<G>(g));
	}


	//
	// an attempt at a '$' operator like haskell, so we can avoid parens:
	//
	//  inc(square(4))
	//  (inc % square)(4)
	//  inc % square << 4
	//
	//template <typename F, typename A>
	//inline decltype(auto) operator << (F&& f, A&& a) {
	//	return std::invoke(std::forward<F>(f), std::forward<A>(a));
	//}

#if 0
	// this style of overloads could be used to dramatically restrict composition, but
	// would require overloads for any functor intended to be composable
	template <typename F, typename G1, typename G2>
	inline decltype(auto) operator % (F&& f, detail::composited_t<G1, G2>&& g) {
		return detail::composited_t<F, decltype(g)>{std::forward<F>(f), std::move(g)}; }
#endif

}
