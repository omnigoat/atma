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
			template <typename F, typename T, typename... Args>
			static inline auto apply(F&& f, T&& t, Args&&... args)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return tuple_applier_t<N - 1>::apply(std::forward<F>(f), std::forward<T>(t),
					std::forward<typename std::tuple_element<N - 1, T>::type>(std::get<N - 1>(std::forward<T>(t))), std::forward<Args>(args)...);
			}
		};

		template <>
		struct tuple_applier_t<0>
		{
			// fnptr
			template <typename R, typename... Params, typename T, typename... Args>
			static inline auto apply(R(*f)(Params...), T&&, Args&&... args)
			-> R
			{
				return (*f)(std::forward<Args>(args)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static inline auto apply(R(C::*f)(Params...), T&&, C* c, Args&&... args)
			-> R
			{
				return (c->*f)(std::forward<Args>(args)...);
			}

			// callable
			template <typename F, typename T, typename... A>
			static inline auto apply(F&& f, T&&, A&&... a)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return f(std::forward<A>(a)...);
			}
		};
	}

	template <typename R, typename... Params, typename Tuple>
	inline auto apply_tuple(R(*f)(Params...), Tuple&& t)
	-> R
	{
		typedef typename std::decay<Tuple>::type DT;
		typedef typename std::remove_reference<Tuple>::type RRT;

		return detail::tuple_applier_t<std::tuple_size<DT>::value>::apply(f, std::forward<RRT>(t));
	}

	template <typename R, typename C, typename... Params, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...), Tuple&& t)
	-> R
	{
		typedef typename std::decay<Tuple>::type DT;
		typedef typename std::remove_reference<Tuple>::type RRT;

		return detail::tuple_applier_t<std::tuple_size<DT>::value>::apply(f, std::forward<RRT>(t));
	}

//=====================================================================
} // namespace xtm
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

