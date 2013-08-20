#ifndef ATMA_XTM_TUPLE_HPP
#define ATMA_XTM_TUPLE_HPP
//=====================================================================
#include <tuple>
#include <atma/xtm/function.hpp>
//=====================================================================
namespace atma {
namespace xtm {
//=====================================================================
	
	namespace detail {
		template <uint32_t N> struct tuple_applier_t;
	}

	//=====================================================================
	// apply_tuple
	// -------------
	//   Takes a callable object and a tuple of arguments, and calls the
	//   object with those arguments. @callable may be a function-pointer,
	//   a member-function-pointer (with the first tuple element being a
	//   pointer to an instantiation of the class), or a generic callable
	//   object, like a std::function<>, or even a lambda.
	//
	//   A helper function is provided for member-function-pointers where
	//   the pointer to the instantiation isn't in the tuple to begin with.
	//   For speeeed.
	//=====================================================================
	template <typename F, typename... Args>
	inline auto apply_tuple(F&& f, std::tuple<Args...>&& t)
	-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		typedef typename std::decay<decltype(t)>::type DT;
		typedef typename std::remove_reference<decltype(t)>::type RRT;

		return detail::tuple_applier_t<std::tuple_size<DT>::value>::apply(std::forward<F>(f), std::forward<RRT>(t));
	}


	template <typename R, typename C, typename... Params, typename... Args>
	inline auto apply_tuple(R(C::*f)(Params...), C* c, std::tuple<Args...>&& t)
	-> R
	{
		typedef typename std::decay<decltype(t)>::type DT;
		typedef typename std::remove_reference<decltype(t)>::type RRT;

		return detail::tuple_applier_t<std::tuple_size<DT>::value>::apply(f, std::forward<RRT>(t), c);
	}





	namespace detail {
		template <uint32_t N>
		struct tuple_applier_t
		{
			template <typename F, typename T, typename... Args>
			static inline auto apply(F&& f, T&& t, Args&&... args)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return tuple_applier_t<N - 1>::apply(std::forward<F>(f), std::forward<T>(t),
					std::forward<typename std::tuple_element<N - 1, T>::type>(std::get<N - 1>(std::forward<T>(t))), std::forward<Args>(args)...);
			}

			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static inline auto apply(R(C::*f)(Params...), T&& t, C* c, Args&&... args)
			-> R
			{
				return tuple_applier_t<N - 1>::apply(f, std::forward<T>(t), c,
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


//=====================================================================
} // namespace xtm
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

