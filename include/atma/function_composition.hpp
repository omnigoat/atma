#pragma once

#include <atma/enable_if.hpp>

#include <utility>

//
//  atma::detail::composited_t
//  ----------------------------
//    a functor that calls one function, then passes that result to another.
//    if possible, it is constructed with a concrete functor-operator. this
//    occurs when G has a concrete functor-operator. otherwise, a templated
//    functor-operator is used.
//
namespace atma { namespace detail {

	template <typename F, typename G> struct composited_t;
	template <typename F, typename G, bool> struct composited_ii_t;
	template <typename F, typename G, typename Args> struct composited_iii_t;
	template <typename F, typename G> struct composited_iii_v_t;


	template <typename F, typename G, typename... Args>
	struct composited_iii_t<F, G, std::tuple<Args...>>
	{
		template <typename FF, typename GG>
		composited_iii_t(FF&& f, GG&& g)
			: f(std::forward<FF>(f)), g(std::forward<GG>(g))
		{}

		decltype(auto) operator ()(Args... args)
		{
			return std::forward<F>(f)(std::forward<G>(g)(std::forward<Args>(args)...));
		}

	private:
		F f;
		G g;
	};

	template <typename F, typename G>
	struct composited_iii_v_t
	{
		template <typename FF, typename GG>
		composited_iii_v_t(FF&& f, GG&& g)
			: f(std::forward<FF>(f)), g(std::forward<GG>(g))
		{}

		template <typename... Args>
		decltype(auto) operator ()(Args&&... args)
		{
			return std::forward<F>(f)(std::forward<G>(g)(std::forward<Args>(args)...));
		}

	private:
		F f;
		G g;
	};

	template <typename F, typename G>
	struct composited_ii_t<F, G, true>
		: composited_iii_t<F, G, typename function_traits<std::decay_t<G>>::tupled_args_type>
	{
		template <typename FF, typename GG>
		composited_ii_t(FF&& f, GG&& g)
			: composited_iii_t{std::forward<FF>(f), std::forward<GG>(g)}
		{}
	};

	template <typename F, typename G>
	struct composited_ii_t<F, G, false>
		: composited_iii_v_t<F, G>
	{
		template <typename FF, typename GG>
		composited_ii_t(FF&& f, GG&& g)
			: composited_iii_v_t{std::forward<FF>(f), std::forward<GG>(g)}
		{}
	};

	template <typename F, typename G>
	struct composited_t : composited_ii_t<F, G, is_callable_v<std::decay_t<G>>>
	{
		template <typename FF, typename GG>
		composited_t(FF&& f, GG&& g)
			: composited_ii_t{std::forward<FF>(f), std::forward<GG>(g)}
		{
		}
	};
} }

namespace atma
{
	template <typename F, typename G>
	struct function_composition_override
	{
		template <typename FF, typename GG>
		static decltype(auto) compose(FF&& f, GG&& g) {
			return detail::composited_t<F, G>{std::forward<FF>(f), std::forward<GG>(g)};
		}
	};

	template <typename F, typename G>
	inline decltype(auto) compose(F&& f, G&& g) {
		return function_composition_override<std::remove_reference_t<F> , std::remove_reference_t<G>>
			::compose(std::forward<F>(f), std::forward<G>(g));
	}





	//
	// composition, like the dot-operator in Haskell, if that floats your boat
	//
	template <typename F, typename G>
	inline decltype(auto) operator % (F&& f, G&& g) {
		return compose(std::forward<F>(f), std::forward<G>(g));
	}


	//
	// an attempt at a '$' operator like haskell, so we can avoid parens:
	//
	//  inc(square(4))
	//  (inc % square)(4)
	//  inc % square << 4
	//
	template <typename F, typename A>
	inline decltype(auto) operator << (F&& f, A&& a) {
		return call_fn(std::forward<F>(f), std::forward<A>(a));
	}

#if 0
	// this style of overloads could be used to dramatically restrict composition, but
	// would require overloads for any functor intended to be composable
	template <typename F, typename G1, typename G2>
	inline decltype(auto) operator % (F&& f, detail::composited_t<G1, G2>&& g) {
		return detail::composited_t<F, decltype(g)>{std::forward<F>(f), std::move(g)}; }
#endif

}
