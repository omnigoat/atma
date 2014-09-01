#pragma once

#include <atma/types.hpp>
#include <atma/xtm/function.hpp>
#include <atma/enable_if.hpp>

#include <tuple>
#include <cstdint>

// msvc does weird things with its intrinsics
#pragma warning(push)
#pragma warning(disable:4305)


namespace atma { namespace xtm {

	template <size_t I> struct placeholder_t {};

}}

namespace
{
	atma::xtm::placeholder_t<0> const arg1;
	atma::xtm::placeholder_t<1> const arg2;
	atma::xtm::placeholder_t<2> const arg3;
}

namespace atma { namespace xtm {
	
	template <size_t Count, typename... Acc>
	struct placeholder_tuple_args_t
	{
		typedef typename placeholder_tuple_args_t<Count - 1, placeholder_t<Count - 1>, Acc...>::type type;
	};

	template <typename... Acc>
	struct placeholder_tuple_args_t<0, Acc...>
	{
		typedef std::tuple<Acc...> type;
	};

	template <typename F, typename... B>
	struct filled_bindings_t
	{
		typedef typename placeholder_tuple_args_t<function_traits<F>::arity - sizeof...(B)>::type trailing_bindings;
		typedef decltype(std::tuple_cat(std::tuple<B...>(), trailing_bindings())) type;
	};

	namespace detail {
		template <size_t N> struct tuple_applier_t;
		template <size_t M> struct bound_tuple_applier_t;
	}


	template <typename F, typename Bindings>
	struct bind_t
	{
		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(std::forward<FF>(fn)), bindings_(std::forward<BB>(bindings))
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args)
		-> typename function_traits<std::decay_t<F>>::result_type
		{
			return apply_tuple_ex(fn_, bindings_, std::forward_as_tuple(args...));
		}

	private:
		F fn_;
		Bindings bindings_;
	};

	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings) -> bind_t<F, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}


	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings)
	-> bind_t<F, typename filled_bindings_t<F, Bindings...>::type>
	{
		auto const f_size = function_traits<F>::arity;
		auto const binding_size = sizeof...(Bindings);

		//static_assert(f_size - binding_size >= 0, "uh-oh");

		auto const rem_args = typename placeholder_tuple_args_t<f_size - binding_size>::type();
		auto merged_bindings = std::tuple_cat(std::make_tuple(bindings...), rem_args);

		//return xtm::bind(f, merged_bindings);
		return bind_t<F, decltype(merged_bindings)>(f, merged_bindings);
	}


	template <typename R, typename from, typename to>
	struct type_if_castible : 
		std::enable_if<
			std::is_convertible<
				std::remove_reference_t<from>&,
				std::remove_reference_t<to>&>::value, R>
				{};


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

	// catch-all
	template <typename F, typename Tuple>
	inline auto apply_tuple(F&& f, Tuple&& xs)
	-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		auto const f_size = function_traits<F>::arity;
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		// member-functions screw with this for the moment
		//static_assert(f_size == tuple_size, "incorrect number of args");

		return detail::tuple_applier_t<tuple_size>::apply(
			std::forward<F>(f),
			std::forward<Tuple>(xs));
	}

	// memfnptr helpers
	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...), CC* c, Tuple&& xs)
	-> typename type_if_castible<R, CC, C>::type
	{
		auto const f_size = sizeof...(Params);
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		static_assert(f_size == tuple_size, "incorrect number of bindings (must match parameter-count)");

		return detail::tuple_applier_t<tuple_size + 1>::apply(
			f,
			std::tuple_cat(std::make_tuple(c), xs));
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...) const, CC* c, Tuple&& xs)
	-> typename type_if_castible<R, CC, C const>::type
	{
		auto const f_size = sizeof...(Params);
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		static_assert(f_size == tuple_size, "incorrect number of bindings (must match parameter-count)");

		return detail::tuple_applier_t<tuple_size + 1>::apply(
			f,
			std::tuple_cat(std::make_tuple(c), xs));
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...), CC&& c, Tuple&& xs)
	-> typename type_if_castible<R, CC, C>::type
	{
		auto const f_size = sizeof...(Params);
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		static_assert(f_size == tuple_size, "incorrect number of bindings (must match parameter-count)");

		return detail::tuple_applier_t<tuple_size + 1>::apply(
			f,
			std::tuple_cat(std::make_tuple(c), xs));
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto apply_tuple(R(C::*f)(Params...) const, CC&& c, Tuple&& xs)
	-> typename type_if_castible<R, CC, C const>::type
	{
		auto const f_size = sizeof...(Params);
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		static_assert(f_size == tuple_size, "incorrect number of bindings (must match parameter-count)");

		return detail::tuple_applier_t<tuple_size + 1>::apply(
			f,
			std::tuple_cat(std::make_tuple(c), xs));
	}


	//=====================================================================
	// apply_tuple_ex
	// ----------------
	//   
	//=====================================================================
	template <typename F, typename Bindings, typename Args>
	inline auto apply_tuple_ex(F&& f, Bindings&& b, Args&& a)
	-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		auto const f_size = function_traits<F>::arity;
		auto const binding_size = std::tuple_size<std::decay_t<Bindings>>::value;

		static_assert(f_size == binding_size, "incorrect number of bindings (must match parameter-count)");

		return detail::bound_tuple_applier_t<binding_size>::apply(
			std::forward<F>(f),
			std::forward<Bindings>(b),
			std::forward<Args>(a));
	}




	namespace detail
	{
		//=====================================================================
		// bound_tuple_applier_t
		//
		//
		//
		//=====================================================================
		template <size_t M>
		struct bound_tuple_applier_t
		{
			template <typename F, typename Bindings, typename Args, typename... Unpacked>
			static auto apply(F&& f, Bindings&& b, Args&& a, Unpacked&&... u)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				auto&& element = select_element(
					std::get<M - 1>(std::forward<Bindings>(b)), std::forward<Args>(a));

				return bound_tuple_applier_t<M - 1>::apply(
					std::forward<F>(f),
					std::forward<Bindings>(b),
					std::forward<Args>(a),
					std::forward<decltype(element)>(element),
					std::forward<Unpacked>(u)...);
			}

		private:
			template <typename Binding, typename Args>
			static auto select_element(Binding&& b, Args&&)
			-> decltype(b)
			{
				return std::forward<Binding>(b);
			}

			template <size_t I, typename Args>
			static auto select_element(placeholder_t<I>, Args&& args)
			-> typename std::tuple_element<I, Args>::type
			{
				return std::get<I>(std::forward<Args>(args));
			}
		};


		template <>
		struct bound_tuple_applier_t<0>
		{
			// callable
			template <typename F, typename Bindings, typename Args, typename... Unpacked>
			static auto apply(F&& f, Bindings&&, Args&&, Unpacked&&... u)
			-> typename atma::xtm::function_traits<std::decay_t<F>>::result_type
			{
				return f(std::forward<Unpacked>(u)...);
			}

			// fnptr
			template <typename R, typename... Params, typename Bindings, typename Args, typename... Unpacked>
			static auto apply(R(*f)(Params...), Bindings&&, Args&&, Unpacked&&... u)
			-> R
			{
				return (*f)(std::forward<Unpacked>(u)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename Bindings, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...) const, Bindings&&, Args&&, CC* c, Unpacked&&... u)
			-> type_if_castible<R, CC, C>
			{
				static_assert(sizeof...(Params) == sizeof...(u));
				return (c->*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Bindings, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...) const, Bindings&&, Args&&, CC& c, Unpacked&&... u)
			-> type_if_castible<R, CC, C>
			{
				return (c.*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Bindings, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...), Bindings&&, Args&&, CC* c, Unpacked&&... u)
			-> type_if_castible<R, CC, C>
			{
				return (c->*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Bindings, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...), Bindings&&, Args&&, CC& c, Unpacked&&... u)
			-> type_if_castible<R, CC, C>
			{
				return (c.*f)(std::forward<Unpacked>(u)...);
			}
		};






		template <size_t N>
		struct tuple_applier_t
		{
			template <typename F, typename Args, typename... Unpacked>
			static auto apply(F&& f, Args&& a, Unpacked&&... u)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				return tuple_applier_t<N - 1>::apply(
					std::forward<F>(f),
					std::forward<Args>(a),
					std::get<N - 1>(a),
					std::forward<Unpacked>(u)...);
			}
		};


		template <>
		struct tuple_applier_t<0>
		{
			// callable
			template <typename F, typename Args, typename... Unpacked>
			static auto apply(F&& f, Args&&, Unpacked&&... a)
				-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
			{
				auto const fsize = xtm::function_traits<F>::arity;
				static_assert(fsize == sizeof...(Unpacked), "incorrect number of arguments");
				return f(std::forward<Unpacked>(a)...);
			}

			// fnptr
			template <typename R, typename... Params, typename Args, typename... Unpacked>
			static auto apply(R(*f)(Params...), Args&&, Unpacked&&... u)
			-> R
			{
				static_assert(sizeof...(Params) == sizeof...(Unpacked), "incorrect number of arguments");
				return (*f)(std::forward<Unpacked>(u)...);
			}

			// memfnptr
			template <typename R, typename C, typename... Params, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...) const, Args&&, CC* c, Unpacked&&... u)
			-> typename type_if_castible<R, CC, C const>::type
			{
				static_assert(sizeof...(Params) == sizeof...(Unpacked), "incorrect number of arguments");
				return (c->*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...) const, Args&&, CC&& c, Unpacked&&... u)
			-> typename type_if_castible<R, CC, C const>::type
			{
				static_assert(sizeof...(Params) == sizeof...(Unpacked), "incorrect number of arguments");
				return (c.*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...), Args&&, CC* c, Unpacked&&... u)
			-> typename type_if_castible<R, CC, C>::type
			{
				static_assert(sizeof...(Params) == sizeof...(Unpacked), "incorrect number of arguments");
				return (c->*f)(std::forward<Unpacked>(u)...);
			}

			template <typename R, typename C, typename... Params, typename Args, typename CC, typename... Unpacked>
			static auto apply(R(C::*f)(Params...), Args&&, CC&& c, Unpacked&&... u)
			-> typename type_if_castible<R, CC, C>::type
			{
				static_assert(sizeof...(Params) == sizeof...(Unpacked), "incorrect number of arguments");
				return (c.*f)(std::forward<Unpacked>(u)...);
			}
		};
	}


} }

#pragma warning(pop)
