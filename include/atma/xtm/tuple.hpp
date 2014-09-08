#pragma once

#include <atma/types.hpp>
#include <atma/xtm/function.hpp>
#include <atma/xtm/idxs.hpp>
#include <atma/enable_if.hpp>

#include <tuple>
#include <cstdint>


// msvc does weird things with its intrinsics
#pragma warning(push)
#pragma warning(disable:4305)


namespace atma { namespace xtm {

	template <int I> struct placeholder_t
	{
		// placeholder_t is an identity function (i.e., it constructs itself)
		using type = placeholder_t<I>;
	};

}}

namespace
{
	atma::xtm::placeholder_t<0> const arg1;
	atma::xtm::placeholder_t<1> const arg2;
	atma::xtm::placeholder_t<2> const arg3;
	atma::xtm::placeholder_t<3> const arg4;
	atma::xtm::placeholder_t<4> const arg5;
	atma::xtm::placeholder_t<5> const arg6;
	atma::xtm::placeholder_t<6> const arg7;
	atma::xtm::placeholder_t<7> const arg8;
}


namespace atma { namespace xtm {

	namespace detail
	{
		template <template <int> class, typename> struct tuple_idxs_map_tx;

		template <uint, typename>     struct tuple_get_tx;
		template <typename, typename> struct tuple_select_tx;

		template <typename> struct tuple_head_tt;
		template <typename> struct tuple_tail_tt;

		template <typename, typename> struct tuple_join_tt;
	}



	//
	//  tuple_idxs_map_t
	//  ------------------
	//    maps a function over an idxs_t and generates a tuple
	//
	//    tuple_idxs_map_t<placeholder_t, idxs_t<0, 1, 2>> ===
	//      std::tuple<placeholder_t<0>, placeholder_t<1>, placeholder_t<2>>
	//
	template <template <int> class f, typename idxs>
	using tuple_idxs_map_t = typename detail::tuple_idxs_map_tx<f, idxs>::type;

	namespace detail
	{
		template <template <int> class f, int... idxs>
		struct tuple_idxs_map_tx<f, idxs_t<idxs...>>
		{
			using type = std::tuple<typename f<idxs>::type...>;
		};
	}




	//
	//  tuple_get
	//  -----------
	//    gets an element from a tuple
	//
	//    tuple_get_t<1, std::tuple<int, float, string, dragon>>
	//      === std::tuple<float>
	//
	template <uint idx, typename xs>
	using tuple_get_t = typename detail::tuple_get_tx<idx, std::decay_t<xs>>::type;

	template <uint idx, typename xs_t>
	inline auto tuple_get(xs_t&& xs) -> tuple_get_t<idx, xs_t>
	{
		return detail::tuple_get_tx<idx, std::decay_t<xs_t>>::call(std::forward<xs_t>(xs));
	}

	namespace detail
	{
		template <uint idx, typename xxs>
		struct tuple_get_tx
		{
			using type = typename std::tuple_element<idx, xxs>::type;

			template <typename xs_t>
			static auto call(xs_t&& xs) -> typename std::tuple_element<idx, std::decay_t<xs_t>>::type
			{
				static_assert(std::is_same<std::decay_t<xs_t>, xxs>::value, "bad types");

				return std::get<idx>(xs);
			}
		};
	}



	template <template <typename...> class f, typename... args>
	struct type_curry_tx
	{
	private:
		template <typename... lhs>
		struct impl : f<lhs..., args...>::type
		{};

	public:
		template <typename... lhs>
		using type = impl<lhs...>;
	};

	template <template <typename...> class f, typename... args>
	using type_curry_t = typename type_curry_tx<f, args...>::type;





	//
	//  tuple_select
	//  ---------------
	//    tuple_select_t<idxs_t<1, 3>, std::tuple<int, float, string, dragon>>
	//      === std::tuple<float, dragon>
	//
	//    tuple_select<ids_t<1>>(std::tuple<float, int>()) == std::tuple<int>()
	//
	template <typename idxs, typename xs_t>
	using tuple_select_t = typename detail::tuple_select_tx<idxs, std::decay_t<xs_t>>::type;

	template <typename idxs, typename xs_t>
	inline auto tuple_select(xs_t&& xs) -> tuple_select_t<idxs, xs_t>
	{
		return detail::tuple_select_tx<idxs, std::decay_t<xs_t>>::call(xs);
	}

	namespace detail
	{
		template <uint... idxs, typename ys>
		struct tuple_select_tx<idxs_t<idxs...>, ys>
		{
			using type = std::tuple<typename detail::tuple_get_tx<idxs, ys>::type...>;

			template <typename xs_t>
			static auto call(xs_t&& xs) -> type
			{
				return std::make_tuple(detail::tuple_get_tx<idxs, typename std::decay<xs_t>::type>::call(std::forward<xs_t>(xs))...);
			}
		};
	}




	//
	//  tuple_head_t
	//  --------------
	//    returns the first type in a tuple
	//
	template <typename xs>
	using tuple_head_t = typename detail::tuple_head_tt<typename std::decay<xs>::type>::type;

	template <typename xs_t>
	inline auto tuple_head(xs_t&& xs) -> tuple_head_t<xs_t>
	{
		return std::get<0>(std::forward<xs_t>(xs));
	}

	namespace detail
	{
		template <typename x, typename... xs>
		struct tuple_head_tt<std::tuple<x, xs...>>
		{
			using type = x;
		};
	}




	//
	//  tuple_tail_t
	//  --------------
	//    returns a tuple of the trailing types of a tuple
	//
	//      tuple_tail_t<std::tuple<int, float, string>> === std::tuple<float, string>
	//
	namespace detail
	{
		template <typename x, typename... xs>
		struct tuple_tail_tt<std::tuple<x, xs...>>
		{
			using type = std::tuple<xs...>;
		};
	}

	template <typename xs>
	using tuple_tail_t = typename detail::tuple_tail_tt<typename std::decay<xs>::type>::type;

	template <typename xs_t>
	inline auto tuple_tail(xs_t&& xs) -> tuple_tail_t<xs_t>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<xs_t>>::value;

		return tuple_select<idxs_range_t<1, tuple_size>>(std::forward<xs_t>(xs));
	}




	//
	//  tuple_join_t
	//  --------------
	//    joins a type to a tuple, on either end
	//
	template <typename lhs, typename rhs>
	using tuple_join_t = typename detail::tuple_join_tt<lhs, rhs>::type;
	
	namespace detail
	{
		template <typename... xs, typename x>
		struct tuple_join_tt<std::tuple<xs...>, x>
		{
			typedef std::tuple<xs..., x> type;
		};

		template <typename... xs, typename x>
		struct tuple_join_tt<x, std::tuple<xs...>>
		{
			typedef std::tuple<x, xs...> type;
		};

		template <typename A, typename B>
		using tuple_join_t = typename tuple_join_tt<A, B>::type;
	}




	//
	//  tuple_cat_t
	//  -------------
	//    the type of two concatenated tuples
	//
	//      tuple_cat_t<std::tuple<int>, std::tuple<float, string>> === std::tuple<int, float, string>
	//
	template <typename Lhs, typename Rhs>
	struct tuple_cat_tt
	{};

	template <typename... Lhs, typename... Rhs>
	struct tuple_cat_tt<std::tuple<Lhs...>, std::tuple<Rhs...>>
	{
		typedef std::tuple<Lhs..., Rhs...> type;
	};

	template <typename Lhs, typename Rhs>
	using tuple_cat_t = typename tuple_cat_tt<Lhs, Rhs>::type;




	//
	//  tuple_placeholder_list_t
	//  tuple_placeholder_range_t
	//  ---------------------------
	//
	template <uint begin, uint end>
	using tuple_placeholder_range_t =
		typename detail::tuple_idxs_map_tx<placeholder_t, typename detail::idxs_range_impl<begin, end>::type>::type;

	template <uint count>
	using tuple_placeholder_list_t =
		typename detail::tuple_idxs_map_tx<placeholder_t, typename detail::idxs_range_impl<0, count>::type>::type;



	



	




	//
	//
	//
	template <template <typename, typename> class F, typename Y, typename XS>
	struct tuple_fold_tt
	{
		using type =
			typename tuple_fold_tt<F, F<Y, tuple_head_t<XS>>, tuple_tail_t<XS>>::type;
	};

	template <template <typename, typename> class F, typename Y>
	struct tuple_fold_tt<F, Y, std::tuple<>>
	{
		using type = Y;
	};




	



	//
	//  tuple_map_t
	//  --------------------
	//
	namespace detail
	{
		template <template <typename...> class f, typename xs>
		struct tuple_map_tt;

		template <template <typename...> class f, typename... xs>
		struct tuple_map_tt<f, std::tuple<xs...>>
		{
			using type = std::tuple<typename f<xs>::type...>;
		};
	}

	template <template <typename> class f, typename xs>
	using tuple_map_t = typename detail::tuple_map_tt<f, xs>::type;



	//
	//  tuple_flip_tt
	//  ---------------
	//    flips the arguments in a tuple:
	//
	//      tuple_flip_t<std::tuple<A, B, C>> === std::tuple<C, B, A>
	//      tuple_flip_t<std::tuple<A>> === std::tuple<A>
	//      tuple_flip_t<std::tuple<>> === std::tuple<>
	//
	template <typename Tuple> struct tuple_flip_tt;
	template <typename Tuple> using tuple_flip_t = typename tuple_flip_tt<Tuple>::type;

#if 0
	template <typename Tuple>
	struct tuple_flip_tt
	{
		using type =
			tuple_join_t<
				typename tuple_flip_tt<tuple_tail_t<Tuple>>::type,
				tuple_head_t<Tuple>>;
	};

	template <typename T>
	struct tuple_flip_tt<std::tuple<T>>
	{
		using type = std::tuple<T>;
	};

	template <>
	struct tuple_flip_tt<std::tuple<>>
	{
		using type = std::tuple<>;
	};

#else
	template <typename>
	struct tuple_flip_tt
	{
		using type = std::tuple<>;
	};

	template <typename head, typename... tail>
	struct tuple_flip_tt<std::tuple<head, tail...>>
	{
		using type =
			tuple_join_t<
				tuple_flip_t<std::tuple<tail...>>,
				head>;
	};

	template <typename T>
	struct tuple_flip_tt<std::tuple<T>>
	{
		using type = std::tuple<T>;
	};
#endif

	//template <typename Tuple>
	//using tuple_flip_t = typename tuple_flip_tt<Tuple>::type;

	//
	//  tuple_placeholder_list_t
	//  tuple_placeholder_range_t
	//  ---------------------
	//
#if 0
	template <uint Rem, uint Idx>
	struct placeholder_list_tt
	{
		using type =
			tuple_join_t<
			typename placeholder_list_tt<Rem - 1, Idx>::type,
			placeholder_t<Idx + Rem - 1>>;
	};

	template <uint Idx>
	struct placeholder_list_tt<0, Idx>
	{
		using type = std::tuple<>;
	};

	template <uint Count>
	using tuple_placeholder_list_t = typename placeholder_list_tt<Count, 0>::type;

	template <uint Begin, uint End>
	using tuple_placeholder_range_t = typename placeholder_list_tt<End - Begin, Begin>::type;
#endif


	//
	//  curried_bindings_t
	//  --------------------
	//    takes a 'callable' and a list of arguments, and returns the completed
	//    bindings for a curried function. i.e:
	//
	//      assuming a function: int plus3(int a, int b, int c) { return a + b + c; }
	//
	//      curried_bindings_t<&plus3, int>    ==  std::tuple<int, placeholder_t<0>, placeholder_t<1>>
	//
	template <typename F, typename... B>
	struct curried_bindings_tt
	{
		using type = tuple_cat_t<
			std::tuple<B...>,
			tuple_placeholder_list_t<function_traits<F>::arity - sizeof...(B)>>;
	};

	template <typename F, typename... B>
	using curried_bindings_t = typename curried_bindings_tt<F, B...>::type;



	template <typename F>
	using flipped_bindings_t =
		tuple_flip_t<curried_bindings_t<F>>;




	namespace detail {
		template <size_t N> struct tuple_applier_t;
		template <size_t M> struct bound_tuple_applier_t;
	}


	//
	// takes bindings and arguments and applies the arguments to the bindings
	//
	template <size_t N, typename Bindings, typename Args>
	struct bound_arguments_tt
	{
	private:
		template <typename Binding, typename AArgs>
		static auto select_element(Binding&& b, AArgs&&)
		-> decltype(b)
		{
			return std::forward<Binding>(b);
		}

		template <size_t I, typename AArgs>
		static auto select_element(placeholder_t<I>, AArgs&& args)
			-> typename std::tuple_element<I, AArgs>::type
		{
			return std::get<I>(std::forward<AArgs>(args));
		}

	public:
		using type =
			tuple_join_t<
				typename bound_arguments_tt<N - 1, Bindings, Args>::type,
				std::remove_reference_t<decltype(select_element(
					std::get<N - 1>(std::declval<Bindings>()),
					std::declval<Args>()))>>;
				

		template <typename BBindings, typename AArgs>
		static auto apply(BBindings&& bindings, AArgs&& args) -> type
		{
			auto&& element = select_element(
				std::get<N - 1>(std::forward<BBindings>(bindings)),
				std::forward<AArgs>(args));

			return std::tuple_cat(
				bound_arguments_tt<N - 1, Bindings, Args>::apply(
					std::forward<BBindings>(bindings),
					std::forward<AArgs>(args)),
				std::make_tuple(element));
		}



	};

	template <typename Bindings, typename Args>
	struct bound_arguments_tt<0, Bindings, Args>
	{
		using type = std::tuple<>;

		static auto apply(Bindings const& bindings, Args const& args) -> type
		{
			return {};
		}
	};


	template <typename Bindings, typename Args>
	using bound_arguments_t = typename bound_arguments_tt<std::tuple_size<
		std::remove_reference_t<Bindings>>::value, Bindings, Args>::type;

	template <typename Bindings, typename Args>
	inline auto bind_arguments(Bindings&& bindings, Args&& args) -> bound_arguments_t<Bindings, Args>
	{
		auto const bindings_count = std::tuple_size<std::remove_reference_t<Bindings>>::value;
		return bound_arguments_tt<bindings_count, Bindings, Args>::apply(
			std::forward<Bindings>(bindings),
			std::forward<Args>(args));
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
		-> typename function_traits<F>::result_type
		{
			return detail::bound_tuple_applier_t<std::tuple_size<decltype(bindings_)>::value>::apply(
				fn_,
				bindings_,
				std::forward_as_tuple(args...));
		}

		auto fn() const -> F const& { return fn_; }
		auto bindings() const -> Bindings const& { return bindings_; }

	private:
		F fn_;
		Bindings bindings_;
	};





	// regular bind
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
		-> bind_t<std::remove_reference_t<F>, std::tuple<Bindings...>>
	{
		return{std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}

	// bind taking a bind compresses the binds
	template <typename PreF, typename Prebindings, typename... Bindings>
	inline auto bind(bind_t<PreF, Prebindings> const& b, Bindings&&... bindings)
	-> bind_t<PreF, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
		return{b.fn(), bind_arguments(b.bindings(), std::forward_as_tuple(bindings...))};
	}

	template <typename PreF, typename Prebindings, typename... Bindings>
	inline auto bind(bind_t<PreF, Prebindings>& b, Bindings&&... bindings)
		-> bind_t<PreF, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
		return{b.fn(), bind_arguments(b.bindings(), std::forward_as_tuple(bindings...))};
	}

	template <typename PreF, typename Prebindings, typename... Bindings>
	inline auto bind(bind_t<PreF, Prebindings>&& b, Bindings&&... bindings)
		-> bind_t<PreF, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
		return{b.fn(), bind_arguments(b.bindings(), std::forward_as_tuple(bindings...))};
	}

	template <typename PreF, typename Prebindings, typename... Bindings>
	inline auto bind(bind_t<PreF, Prebindings> const&& b, Bindings&&... bindings)
		-> bind_t<PreF, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
		return{b.fn(), bind_arguments(b.bindings(), std::forward_as_tuple(bindings...))};
	}

	
	
	// curry is a type of bind
	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings)
	-> bind_t<F, curried_bindings_t<F, Bindings...>>
	{
		auto const f_size = function_traits<F>::arity;
		auto const binding_size = sizeof...(Bindings);

		auto const rem_args = tuple_placeholder_list_t<f_size - binding_size>();
		auto merged_bindings = std::tuple_cat(std::make_tuple(bindings...), rem_args);

		return {f, merged_bindings};
	}


	// flip is here?
	template <typename F>
	inline auto flip(F&& f)
	-> bind_t<F, tuple_flip_t<curried_bindings_t<F>>>
	{
		return {f, tuple_flip_t<curried_bindings_t<F>>()};
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
