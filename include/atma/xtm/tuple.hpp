#pragma once

#include <atma/types.hpp>
#include <atma/xtm/function.hpp>
#include <atma/enable_if.hpp>

#include <tuple>
#include <cstdint>
//=====================================================================
namespace atma {
namespace xtm {
//=====================================================================
	
	namespace detail {
		template <size_t N> struct tuple_applier_t;
		template <size_t M> struct bound_tuple_applier_t;
	}


#if 0
	template <typename F, typename... Bindings>
	struct bind_t
	{
		bind_t(F&& fn, Bindings... bindings)
			: fn_(fn), bindings_(bindings...)
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args)
		-> typename function_traits<std::decay_t<F>>::result_type
		{
			return apply_tuple_ex(fn_, std::forward<decltype(bindings_)>(bindings_),
				std::forward<std::tuple<Args...>>(std::tuple<Args...>(std::forward<Args>(args)...)));
		}

	private:
		F fn_;
		std::tuple<typename std::remove_reference<Bindings>::type...> bindings_;
	};

	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings) -> bind_t<F, Bindings...>
	{
		return bind_t<F, Bindings...>(std::forward<F>(f), std::forward<Bindings>(bindings)...);
	}
#endif

	#pragma warning(push)
	#pragma warning(disable:4305)
	template <typename R, typename from, typename to>
	using type_if_convertible_t = typename std::enable_if<std::is_same<from, to>::value, R>::type;

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
#if 0
	template <typename F, typename Tuple>
	inline auto apply_tuple(F&& f, Tuple&& xs)
	-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		return detail::tuple_applier_t<tuple_size>::apply(
			std::forward<F>(f),
			std::forward<Tuple>(xs));
	}
#endif

#if 0
	template <typename R, typename C, typename... Params, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...), C* c, Tuple&& xs)
	-> R
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		return detail::tuple_applier_t<tuple_size>::apply(
			f,
			std::forward<Tuple>(xs),
			c);
	}
#endif

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...), CC&& c, Tuple&& xs)
	-> type_if_convertible_t<R, std::remove_reference_t<CC>, std::remove_reference_t<C>>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		return detail::tuple_applier_t<tuple_size>::apply(
			f,
			std::forward<Tuple>(xs),
			std::forward<CC>(c));
	}

#if 0
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
#endif


	namespace detail
	{
#if 0
		template <size_t I> struct xtm_ph {};


		//=====================================================================
		// bound_tuple_applier_t
		//=====================================================================
		// bindings and values still present
		template <size_t M>
		struct bound_tuple_applier_t
		{
			template <typename F, typename B, typename T, typename... Args>
			static auto apply(F&& f, B&& b, T&& t, Args&&... args)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				// binding element type
				auto&& element = select_element(
					std::get<M - 1>(std::forward<B>(b)), std::forward<T>(t));

				return bound_tuple_applier_t<M - 1>::apply(
					std::forward<F>(f),
					std::forward<B>(b),
					std::forward<T>(t),
					std::forward<decltype(element)>(element),
					std::forward<Args>(args)...);
			}

		private:
			// generic types, pass the binding value through
			template <typename A, typename T>
			static auto select_element(A&& a, T&& b)
				-> A&&
			{
				return std::forward<A>(a);
			}

			// when a placeholder is passed in, pick the argument in that slot
			template <size_t I, typename T>
			static auto select_element(xtm_ph<I>&&, T&& t)
				-> typename std::tuple_element<I, T>::type&&
			{
				return std::forward<typename std::tuple_element<I, T>::type>
					(std::get<I>(std::forward<T>(t)));
			}
		};

		// termination
		template <>
		struct bound_tuple_applier_t<false>
		{
			// fnptr
			template <typename R, typename... Params, typename B, typename T, typename... Args>
			static auto apply(R(*f)(Params...), B&&, T&&, Args&&... args)
			-> R
			{
				return (*f)(std::forward<Args>(args)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename B, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...), B&&, T&&, C* c, Args&&... args)
			-> R
			{
				return (c->*f)(std::forward<Args>(args)...);
			}

			template <typename R, typename C, typename... Params, typename B, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...), B&&, T&&, C& c, Args&&... args)
			-> R
			{
				return (c.*f)(std::forward<Args>(args)...);
			}

			// callable
			template <typename F, typename B, typename T, typename... A>
			static auto apply(F&& f, B&&, T&&, A&&... a)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return f(std::forward<A>(a)...);
			}
		};
#endif

		template <size_t N>
		struct tuple_applier_t
		{
#if 0
			// callable
			template <typename F, typename T, typename... Args>
			static auto apply(F&& f, T&& xs, Args&&... acc)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				// 'acc' will become the expanded list of arguments
				return tuple_applier_t<N - 1>::apply(
					std::forward<F>(f),
					std::forward<T>(xs),
					std::get<N - 1>(xs),
					std::forward<Args>(acc)...);
			}
#endif
			// memfunptr
#if 0
			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...), T&& xs, C* c, Args&&... acc)
			-> R
			{
				return tuple_applier_t<N - 1>::apply(
					f,
					std::forward<T>(xs),
					c,
					std::get<N - 1>(xs),
					std::forward<Args>(acc)...);
			}
#endif

			template <typename R, typename C, typename... Params, typename T, typename CC, typename... Args>
			static auto apply(R(C::*f)(Params...), T&& xs, CC&& c, Args&&... acc)
			-> type_if_convertible_t<R, std::remove_reference_t<CC>, std::remove_reference_t<C>>
			{
				return R();
#if 0
				return tuple_applier_t<N - 1>::apply(
					f,
					std::forward<T>(xs),
					std::forward<CC>(c),
					std::get<N - 1>(xs),
					std::forward<Args>(acc)...);
#endif
			}
		};

#if 0
		template <>
		struct tuple_applier_t<0>
		{
			// fnptr
			template <typename R, typename... Params, typename T, typename... Args>
			static auto apply(R(*f)(Params...), T&&, Args&&... args)
			-> R
			{
				return (*f)(std::forward<Args>(args)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...), T&&, C* c, Args&&... args)
			-> R
			{
				return (c->*f)(std::forward<Args>(args)...);
			}

			template <typename R, typename C, typename... Params, typename T, typename CC, typename... Args>
			static auto apply(R(C::*f)(Params...), T&&, CC&& c, Args&&... args)
			-> type_if_convertible_t<R, CC, C>
			{
				return R();
				//return (c.*f)(std::forward<Args>(args)...);
			}

			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...) const, T&&, C const* c, Args&&... args)
				-> R
			{
				return (c->*f)(std::forward<Args>(args)...);
			}

			template <typename R, typename C, typename... Params, typename T, typename... Args>
			static auto apply(R(C::*f)(Params...) const, T&&, C const& c, Args&&... args)
				-> R
			{
				return (c.*f)(std::forward<Args>(args)...);
			}

			// callable
			template <typename F, typename T, typename... A>
			static auto apply(F&& f, T&&, A&&... a)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return f(std::forward<A>(a)...);
			}
		};
#endif
	}

	#pragma warning(pop)

} }
