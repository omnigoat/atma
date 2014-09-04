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
		
	//
	//  tuple_cat_t
	//  -------------
	//    the type of two concatenated tuples
	//
	//      tuple_cat_t<std::tuple<int>, std::tuple<float, string>>
	//         === std::tuple<int, float, string>
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
	//  placeholder_list_t
	//  --------------------
	//    generates a std::tuple<> of atma::placeholder<>, like so [ex]:
	//      std::tuple<placeholder_t<0>, placeholder_t<1>, placeholder_t<2>>
	//
	template <uint Count, typename... Acc>
	struct placeholder_list_tt
	{
		typedef typename placeholder_list_tt<Count - 1, placeholder_t<Count - 1>, Acc...>::type type;
	};

	template <typename... Acc>
	struct placeholder_list_tt<0, Acc...>
	{
		typedef std::tuple<Acc...> type;
	};

	template <uint Count>
	using placeholder_list_t = typename placeholder_list_tt<Count>::type;



	//
	//  placeholder_list_desc_t
	//  -------------------------
	//    generates a std::tuple<> of descinding atma::placeholder<>, like so [ex]:
	//      std::tuple<placeholder_t<2>, placeholder_t<1>, placeholder_t<0>>
	//
	template <uint Idx, uint Count, typename... Acc>
	struct placeholder_list_desc_tt
	{
		typedef typename placeholder_list_desc_tt<Idx + 1, Count - 1, placeholder_t<Idx>, Acc...>::type type;
	};

	template <uint Idx, typename... Acc>
	struct placeholder_list_desc_tt<Idx, 0, Acc...>
	{
		typedef std::tuple<Acc...> type;
	};

	template <uint Count>
	using placeholder_list_desc_t = typename placeholder_list_desc_tt<0, Count>::type;




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
			placeholder_list_t<function_traits<F>::arity - sizeof...(B)>>;
	};

	template <typename F, typename... B>
	using curried_bindings_t = typename curried_bindings_tt<F, B...>::type;



	template <typename F>
	struct flipped_bindings_tt
	{
		using type = placeholder_list_desc_t<function_traits<F>::arity>;
	};

	template <typename F>
	using flipped_bindings_t = typename flipped_bindings_tt<F>::type;



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
#if 0
		template <typename Binding, typename AArgs>
		static auto select_element(Binding&& b, AArgs&&)
		-> decltype(b)
		{
			return std::forward<Binding>(b);
		}
#endif

		template <size_t I, typename AArgs>
		static auto select_element(placeholder_t<I>, AArgs&& args)
			-> typename std::tuple_element<I, AArgs>::type
		{
			return std::get<I>(std::forward<AArgs>(args));
		}

	public:
		using type =
			tuple_cat_t<
				typename bound_arguments_tt<N - 1, Bindings, Args>::type,
				decltype(std::make_tuple(select_element(
					std::get<N - 1>(std::declval<Bindings>()),
					std::declval<Args>())))>;
				

		template <typename BBindings, typename AArgs>
		static auto apply(BBindings&& bindings, AArgs&& args) -> type
		{
			auto&& element = select_element(
				std::get<N - 1>(std::forward<BBindings>(bindings)),
				std::forward<AArgs>(args));

#if 0
			return std::make_tuple(arg1, element);
#else
			return std::tuple_cat(
				bound_arguments_tt<N - 1, Bindings, Args>::apply(
					std::forward<BBindings>(bindings),
					std::forward<AArgs>(args)),
				std::make_tuple(element));
#endif
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
			return apply_tuple_ex(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> F const& { return fn_; }
		auto bindings() const -> Bindings const& { return bindings_; }

	private:
		F fn_;
		Bindings bindings_;
	};

#if 0
	// when bind_t is given another bind_t, we unwrap the first bind_t and rework
	// ourselves to point to the original function with the corrected bindings. this
	// saves a function-call
	template <typename PreF, typename Prebindings, typename Bindings>
	struct bind_t<bind_t<PreF, Prebindings>, Bindings>
	{
		template <typename... Args>
		auto operator ()(Args&&... args)
		//-> typename function_traits<std::decay_t<PreF>>::result_type
		-> std::result_of_t<PreF>
		{
			return std::result_of_t<PreF>();
			//return typename function_traits<std::decay_t<PreF>>::result_type();
			//return apply_tuple_ex(fn_, bindings_, std::forward_as_tuple(args...));
		}

	public:
		PreF fn_;
		bound_arguments_t<Bindings, Prebindings> bindings_;
	};
#endif








	template <typename PreF, typename Prebindings, typename... Bindings>
	inline auto bind(bind_t<PreF, Prebindings> const& b, Bindings&&... bindings)
	-> bind_t<PreF, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
		return{b.fn(), bind_arguments(b.bindings(), std::forward_as_tuple(bindings...))};
	}

	#if 0
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
	-> bind_t<F, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}
	#endif


	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings)
	-> bind_t<F, curried_bindings_t<F, Bindings...>>
	{
		auto const f_size = function_traits<F>::arity;
		auto const binding_size = sizeof...(Bindings);

		auto const rem_args = placeholder_list_t<f_size - binding_size>();
		auto merged_bindings = std::tuple_cat(std::make_tuple(bindings...), rem_args);

		return {f, merged_bindings};
	}

	template <typename F, typename Prebindings, typename... Bindings>
	inline auto curry(bind_t<F, Prebindings> const& b, Bindings&&... bindings)
		-> bind_t<F, bound_arguments_t<Prebindings, std::tuple<Bindings...>>>
	{
	#if 0
		auto const f_size = function_traits<F>::arity;
		auto const binding_size = sizeof...(Bindings);

		auto const rem_args = placeholder_list_t<f_size - binding_size>();
		auto merged_bindings = std::tuple_cat(std::make_tuple(bindings...), rem_args);
	#endif

		return {f};
	}



	template <typename F>
	inline auto flip(F&& f)
	-> bind_t<F, flipped_bindings_t<F>>
	{
		return {f, flipped_bindings_t<F>()};
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
