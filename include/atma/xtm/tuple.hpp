#ifndef ATMA_XTM_TUPLE_HPP
#define ATMA_XTM_TUPLE_HPP
//=====================================================================
#include <tuple>
#include <atma/xtm/function.hpp>
//=====================================================================
namespace atma {
namespace xtm {
//=====================================================================
	

	//=====================================================================
	// apply_tuple
	// -------------
	//   takes a function and a tuple, and calls the function with the
	//   tuple's elements as arguments.
	//=====================================================================
	namespace detail 
	{
		template <size_t N>
		struct tuple_applier_t
		{
			template <typename F, typename T, typename... A>
			static inline auto apply(F&& f, T&& t, A&&... a)
			-> typename function_traits<typename std::decay<F>::type>::result_type
			{
				return tuple_applier_t<N-1>::apply(f, std::forward<T>(t),
					std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...);
			}
		};
		
		template <>
		struct tuple_applier_t<0>
		{
			template <typename F, typename T, typename... A>
			static inline auto apply(F&& f, T&&, A&&... a)
			-> typename function_traits<typename std::decay<F>::type>::result_type
			{
				return  (*f)(std::forward<A>(a)...);
			}

			template <typename R, typename... Params, typename T, typename... A>
			static inline auto apply(R (*f)(Params...), T&&, A&&... a)
			-> R
			{
				return (*f)(std::forward<A>(a)...);
			}

			template <typename R, typename C, typename... Params, typename T, typename... A>
			static inline auto apply(R(C::*f)(Params...), T&&, C&& c, A&&... a)
			-> R
			{
				return (c.*f)(std::forward<A>(a)...);
			}
		};
	}

	template <typename R, typename... Params>
	inline auto apply_tuple(R (*f)(Params...), std::tuple<Params...>&& t)
	-> R
	{
		return detail::tuple_applier_t<std::tuple_size<typename std::decay<decltype(t)>::type>::value>
		  ::apply(f, std::forward<decltype(t)>(t));
	}

	template <typename R, typename C, typename... Params>
	inline auto apply_tuple(R(C::*f)(Params...), std::tuple<C, Params...>&& t)
	-> R
	{
		return detail::tuple_applier_t<std::tuple_size<typename std::decay<decltype(t)>::type>::value>
		  ::apply(f, std::forward<decltype(t)>(t));
	}

//=====================================================================
} // namespace xtm
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

