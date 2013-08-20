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
		template <uint32_t M> struct bound_tuple_applier_t;
	}

	template <typename F, typename... Bindings>
	struct bind_t
	{
		bind_t(F&& fn, Bindings&&... bindings)
			: fn_(fn), bindings_(std::forward<Bindings>(bindings)...)
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args)
		-> typename function_traits<typename std::decay<F>::type>::result_type
		{
			return apply_tuple_ex(fn_, std::forward<decltype(bindings_)>(bindings_), std::make_tuple(std::forward<Args>(args)...));
		}

	private:
		F fn_;
		std::tuple<Bindings...> bindings_;
	};

	template <typename F, typename... Bindings>
	auto bind(F&& f, Bindings&&... bindings) -> bind_t<F, Bindings...>
	{
		return bind_t<F, Bindings...>(std::forward<F>(f), std::forward<Bindings>(bindings)...);
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

	template <typename F, typename... Bindings, typename... Args>
	inline auto apply_tuple_ex(F&& f, std::tuple<Bindings...>&& b, std::tuple<Args...>&& t)
	-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		typedef typename std::decay<decltype(t)>::type DT;
		typedef typename std::decay<decltype(b)>::type DB;
		
		return detail::bound_tuple_applier_t<sizeof...(Bindings)>::apply(
			std::forward<F>(f),
			std::forward<std::remove_reference<decltype(b)>::type>(b),
			std::forward<std::remove_reference<decltype(t)>::type>(t));
	}



	namespace detail
	{
		template <uint32_t I> struct xtm_ph {};


		//=====================================================================
		// select_element_t
		//=====================================================================
		// generic types, pass the binding value through
		template <typename A, typename T>
		struct select_element_t {
			static inline auto apply(A&& a, T&& b) -> A&& {
				return std::forward<A>(a);
			}
		};

		// when a placeholder is passed in, pick the argument in that slot
		template <uint32_t I, typename T>
		struct select_element_t<xtm_ph<I>, T> {
			static inline auto apply(xtm_ph<I>&&, T&& t) -> typename std::tuple_element<I, T>::type&& {
				return std::forward<typename std::tuple_element<I, T>::type>
					(std::get<I>(std::forward<T>(t)));
			}
		};

		
		//=====================================================================
		// bound_tuple_applier_t
		//=====================================================================
		// bindings and values still present
		template <uint32_t M>
		struct bound_tuple_applier_t
		{
			template <typename F, typename B, typename T, typename... Args>
			static inline auto apply(F&& f, B&& b, T&& t, Args&&... args)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				// binding element type
				//typedef typename  B_T;
				auto&& element = 
					select_element_t<typename std::decay<typename std::tuple_element<M - 1, B>::type>::type, T>
					::apply(std::forward<typename std::decay<typename std::tuple_element<M - 1, B>::type>::type>
						(std::get<M - 1>(std::forward<B>(b))), std::forward<T>(t));

				return bound_tuple_applier_t<M - 1>::apply(
					std::forward<F>(f),
					std::forward<B>(b),
					std::forward<T>(t),
					element,
					std::forward<Args>(args)...);
			}
		};

		// termination
		template <>
		struct bound_tuple_applier_t<0>
		{
			// fnptr
			template <typename R, typename... Params, typename B, typename T, typename... Args>
			static inline auto apply(R(*f)(Params...), B&&, T&&, Args&&... args)
			-> R
			{
				return (*f)(std::forward<Args>(args)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename B, typename T, typename... Args>
			static inline auto apply(R(C::*f)(Params...), B&&, T&&, C* c, Args&&... args)
			-> R
			{
				return (c->*f)(std::forward<Args>(args)...);
			}

			// callable
			template <typename F, typename B, typename T, typename... A>
			static inline auto apply(F&& f, B&&, T&&, A&&... a)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return f(std::forward<A>(a)...);
			}
		};



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

