#pragma once

#include <atma/enable_if.hpp>

#include <utility>


namespace atma
{
	namespace detail
	{
		struct composited_base_t {};

		template <typename F, typename G>
		struct composited_t : composited_base_t
		{
			template <typename FF, typename GG>
			composited_t(FF&& f, GG&& g)
				: f(std::forward<FF>(f)), g(std::forward<GG>(g))
			{}

			template <typename... Args>
			auto operator ()(Args&&... args) -> decltype(auto)
			{
				return std::forward<F>(f)(std::forward<G>(g)(std::forward<Args>(args)...));
			}

		private:
			F f;
			G g;
		};
	}


	template <typename F, typename G>
	inline decltype(auto) operator % (F&& f, G&& g) {
		return detail::composited_t<F, G>{std::forward<F>(f), std::forward<G>(g)};
	}


#if 0
	// this style of overloads could be used to dramatically restrict composition, but
	// would require overloads for any functor intended to be composable
	template <typename F, typename G1, typename G2>
	inline decltype(auto) operator % (F&& f, detail::composited_t<G1, G2>&& g) {
		return detail::composited_t<F, decltype(g)>{std::forward<F>(f), std::move(g)}; }
#endif

}
