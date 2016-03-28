#pragma once


#include <atma/tuple.hpp>
#include <atma/placeholders.hpp>
#include <atma/function_traits.hpp>


namespace atma {

	//
	//  curried_bindings_t
	//  --------------------
	//    takes a 'callable' and a list of arguments, and returns the completed
	//    bindings for a curried function. i.e:
	//
	//      assuming a function: int plus3(int a, int b, int c) { return a + b + c; }
	//
	//      curried_bindings_t<&plus3, int>  ===  std::tuple<int, placeholder_t<0>, placeholder_t<1>>
	//      curried_bindings_t<&plus3, int, int>  ===  std::tuple<int, int, placeholder_t<0>>
	//
	namespace detail
	{
		template <typename F, typename... B>
		struct curried_bindings_tx
		{
			static size_t const fn_arity      = function_traits<F>::arity;
			static bool   const fn_ismemfn    = function_traits<F>::is_memfnptr;
			static size_t const bindings_size = sizeof...(B);

			using type = tuple_cat_t<
				std::tuple<B...>,
				tuple_placeholder_list_t<fn_arity + (int)fn_ismemfn - (int)bindings_size>>;
		};
	}

	template <typename F, typename... B>
	using curried_bindings_t = typename detail::curried_bindings_tx<F, B...>::type;




	//
	//  detail::select_bound_arg
	//  --------------------------
	//    chooses the correct binding/argument:
	//
	//      select_bound_arg(2, std::make_tuple(4, 5)) == 2
	//      select_bound_arg(placeholder_t<1>(), std::make_tuple(4, 5)) == 5
	//
	namespace detail
	{
		template <typename Binding, typename Args>
		static auto select_bound_arg(Binding&& b, Args&&)
			-> decltype(b)
		{
			return std::forward<Binding>(b);
		}

		template <size_t I, typename Args>
		static auto select_bound_arg(placeholder_t<I>, Args&& args)
			-> typename std::tuple_element<I, Args>::type
		{
			return std::get<I>(std::forward<Args>(args));
		}
	}




	//
	//  bind_arguments
	//  ----------------
	//    takes a tuple of bindings, and a tuple of arguments, and returns a
	//    resultant tuple of the binding:
	//
	//      bind_arguments(make_tuple(4, 5, arg2, arg1), make_tuple(7, 6)) === {4, 5, 6, 7}
	//
	namespace detail
	{
		template <typename Bindings, typename Args, typename>
		struct bound_arguments_tx;

		template <typename Bindings, typename Args, size_t... Idxs>
		struct bound_arguments_tx<Bindings, Args, idxs_t<Idxs...>>
		{
			using type =
				decltype(std::forward_as_tuple(select_bound_arg(std::get<Idxs>(std::declval<Bindings>()), Args())...));
		};

		template <typename Bindings, typename Args, size_t... Idxs>
		inline auto bind_arguments_impl(Bindings&& bindings, Args&& args, idxs_t<Idxs...>)
			-> typename bound_arguments_tx<Bindings, Args, idxs_t<Idxs...>>::type
		{
			return std::forward_as_tuple(
				select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
		}
	}

	template <typename Bindings, typename Args>
	using bound_arguments_t =
		typename detail::bound_arguments_tx<Bindings, Args, idxs_list_t<std::tuple_size<std::decay_t<Bindings>>::value>>::type;

	template <typename Bindings, typename Args>
	inline auto bind_arguments(Bindings&& bindings, Args&& args) -> bound_arguments_t<Bindings, Args>
	{
		auto const bindings_count = std::tuple_size<std::remove_reference_t<Bindings>>::value;

		return detail::bind_arguments_impl(
			std::forward<Bindings>(bindings),
			std::forward<Args>(args),
			idxs_list_t<bindings_count>());
	}


	//
	//  something
	//  ----------------
	//
	//
	//
	namespace detail
	{
		template <typename Bindings>
		struct bindings_count_tx;

		template <int I, typename... Bindings>
		struct bindings_count_tx<std::tuple<placeholder_t<I>, Bindings...>> {
			static int const value = 1 + bindings_count_tx<std::tuple<Bindings...>>::value;
		};

		template <int I, typename... Bindings>
		struct bindings_count_tx<std::tuple<placeholder_t<I> const, Bindings...>> {
			static int const value = 1 + bindings_count_tx<std::tuple<Bindings...>>::value;
		};

		template <typename X, typename... Bindings>
		struct bindings_count_tx<std::tuple<X, Bindings...>>
		{
			static int const value = bindings_count_tx<std::tuple<Bindings...>>::value;
		};

		template <>
		struct bindings_count_tx<std::tuple<>>
		{
			static int const value = 0;
		};


		template <typename Bindings>
		struct highest_binding_tx;

		template <int I, typename... Bindings>
		struct highest_binding_tx<std::tuple<placeholder_t<I> const, Bindings...>>
		{
			static int const value = I > highest_binding_tx<std::tuple<Bindings...>>::value ? I : highest_binding_tx<std::tuple<Bindings...>>::value;
		};

		template <typename X, typename... Bindings>
		struct highest_binding_tx<std::tuple<X, Bindings...>>
		{
			static int const value = highest_binding_tx<std::tuple<Bindings...>>::value;
		};

		template <>
		struct highest_binding_tx<std::tuple<>>
		{
			static int const value = 0;
		};

		template <typename Args, typename Bindings, int Bidx, int Idx>
		struct original_args_type_t;

		template <typename Args, int Bidx, int Idx, typename X, typename... Bs>
		struct original_args_type_t<Args, std::tuple<X, Bs...>, Bidx, Idx>
		{
			using type = typename original_args_type_t<Args, std::tuple<Bs...>, Bidx + 1, Idx>::type;
		};

		template <typename Args, int Bidx, int Midx, typename... Bs>
		struct original_args_type_t<Args, std::tuple<placeholder_t<Midx> const, Bs...>, Bidx, Midx>
		{
			using type = tuple_get_t<Bidx, Args>;
		};

		//template <typename Args, int Idx, int Bidx, typename... Bs>
		//struct original_args_type_t<Args, std::tuple<placeholder_t<Bidx> const, Bs...>, Idx>
		//{
		//	using type = typename std::conditional<Idx == Bidx,
		//		tuple_get_t<Idx, Args>,
		//		typename original_args_type_t<Args, std::tuple<Bs...>, Idx>::type>::type;
		//};

		template <typename A, int I, int B>
		struct original_args_type_t<A, std::tuple<>, B, I>
		{
			// uh oh!
		};

		template <typename Args, typename Bindings, int Idx, int End>
		struct resultant_args_ii_t
		{
			static_assert(End != 0, "bad End");

			using type = typename tuple_push_front_t<
				typename resultant_args_ii_t<Args, Bindings, Idx + 1, End>::type,
				typename original_args_type_t<Args, Bindings, 0, Idx>::type>;
		};

		template <typename Args, typename Bindings, int Fin>
		struct resultant_args_ii_t<Args, Bindings, Fin, Fin>
		{
			using type = std::tuple<>;
		};

		template <typename Args, typename Bindings>
		struct resultant_args_t
		{
			using type = typename resultant_args_ii_t<Args, Bindings,
				0, highest_binding_tx<Bindings>::value + 1>::type;
		};



	}

	template <typename Bindings>
	using bindings_count_t = detail::bindings_count_tx<Bindings>;

	template <typename Bindings>
	using highest_binding_t = detail::highest_binding_tx<Bindings>;








	template <typename R, typename from, typename to>
	struct type_if_castible :
		std::enable_if<std::is_convertible<std::remove_reference_t<from>&, std::remove_reference_t<to>&>::value, R>
	{};

	template <typename R, typename From, typename To>
	using type_if_castible_t = typename type_if_castible<R, From, To>::type;




	//
	//  call_fn
	//  ---------
	//    takes a callable object and a variadic list of arguments, and calls the
	//    object with those arguments. @f may be a function-pointer,
	//    a member-function-pointer (with the first argument being a
	//    pointer/ref to an instantiation of the class), or a generic callable
	//    object, like a std::function<>, or even a lambda.
	//
	template <typename F, typename... Args>
	auto call_fn(F&& f, Args&&... args) -> result_of_t<F(Args...)>
	{
		return f(std::forward<Args>(args)...);
	}

	template <typename R, typename... Params, typename... Args>
	auto call_fn(R(*fn)(Params...), Args&&... args) -> R
	{
		return (*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...), C& c, Args&&... args) -> R
	{
		return (c.*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...), C&& c, Args&&... args) -> R
	{
		return (c.*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...), C* c, Args&&... args) -> R
	{
		return (c->*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...) const, C& c, Args&&... args) -> R
	{
		return (c.*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...) const, C const& c, Args&&... args) -> R
	{
		return (c.*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...) const, C&& c, Args&&... args) -> R
	{
		return (c.*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...) const, C* c, Args&&... args) -> R
	{
		return (c->*fn)(std::forward<Args>(args)...);
	}

	template <typename C, typename R, typename... Params, typename... Args>
	auto call_fn(R(C::*fn)(Params...) const, C const* c, Args&&... args) -> R
	{
		return (c->*fn)(std::forward<Args>(args)...);
	}


	//
	//  call_fn_tuple
	//  ---------------
	//    Takes a callable object and a tuple of arguments, and calls the
	//    object with those arguments. @f may be a function-pointer,
	//    a member-function-pointer (with the first tuple element being a
	//    pointer/ref to an instantiation of the class), or a generic callable
	//    object, like a std::function<>, or even a lambda.
	//  
	//    A helper function is provided for member-function-pointers where
	//    the pointer to the instantiation isn't in the tuple to begin with.
	//    For speeeed.
	//
	namespace detail
	{
		template <typename F, typename Tuple, size_t... Idxs>
		inline auto call_fn_tuple_impl(F&& f, Tuple&& xs, idxs_t<Idxs...>) -> decltype(auto)
		{
			return call_fn(std::forward<F>(f), std::get<Idxs>(std::forward<Tuple>(xs))...);
		}
	}

	// catch-all
	template <typename F, typename Tuple>
	inline auto call_fn_tuple(F&& f, Tuple&& xs) -> decltype(auto)
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		return detail::call_fn_tuple_impl(std::forward<F>(f), std::forward<Tuple>(xs), idxs_list_t<tuple_size>());
	}

	// memfnptr helpers
	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...), CC* c, Tuple&& xs) -> type_if_castible_t<R, CC, C>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC* c, Tuple&& xs) -> type_if_castible_t<R, CC, C const>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...), CC&& c, Tuple&& xs) -> type_if_castible_t<R, CC, C>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC&& c, Tuple&& xs) -> type_if_castible_t<R, CC, C const>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}




	//
	//  call_fn_bound_tuple
	//  ----------------
	//    takes a function, a tuple of bindings, and a tuple of arguments, and calls
	//    the function after "unwrapping" the bindings/args, example given:
	//
	//    int sub(int x, int y) { return x - y; }
	//
	//    call_fn_bound_tuple(&sub, std::make_tuple(arg2, 2), std::make_tuple(4, 5))  ===  3
	//
	namespace detail
	{
		template <typename F, typename Bindings, typename Args, size_t... Idxs>
		inline auto call_fn_bound_tuple_impl(F&& f, Bindings&& bindings, Args&& args, idxs_t<Idxs...>)
			-> decltype(call_fn(std::forward<F>(f), select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...))
		{
			return call_fn(std::forward<F>(f), select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
		}
	}

	template <typename F, typename Bindings, typename Args>
	inline auto call_fn_bound_tuple(F&& f, Bindings&& b, Args&& a)
		-> decltype(detail::call_fn_bound_tuple_impl(std::forward<F>(f), std::forward<Bindings>(b), std::forward<Args>(a), idxs_list_t<std::tuple_size<std::decay_t<Bindings>>::value>()))
	{
		// this following code won't work for a functor that has overloaded operator (),
		// beacuse function_traits won't be able to auto-magically guess the traits. if this
		// becomes a problem, comment out everything save the return-statement.
		auto const param_size = function_traits<F>::arity + (size_t)function_traits<F>::is_memfnptr;
		auto const binding_size = std::tuple_size<std::decay_t<Bindings>>::value;

		static_assert(param_size == binding_size, "incorrect number of bindings (must match parameter-count)");
		
		return detail::call_fn_bound_tuple_impl(
			std::forward<F>(f),
			std::forward<Bindings>(b),
			std::forward<Args>(a),
			idxs_list_t<binding_size>());
	}




	//
	//  bind
	//  ------
	//    takes a function, and a tuple of bindings, and returns a functor taking 
	//
	//
	template <typename F, typename BindingsRef> //, typename... Args>
	struct bind_t
	{
		// bind doesn't take anything by reference, so we just straight-up store
		// by value. this will still perform construct-by-reference (rvalue/lvalue)
		using function_t = std::decay_t<F>;
		using bindings_t = tuple_map_t<std::decay, BindingsRef>;

		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(std::forward<FF>(fn))
			, bindings_(std::forward<BB>(bindings))
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args) -> decltype(call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...)))
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> function_t const& { return fn_; }
		auto bindings() const -> bindings_t const& { return bindings_; }

	private:
		function_t fn_;
		bindings_t bindings_;
	};

#if 0
	template <typename F, typename BindingsRef>
	struct bind_t //: bind_base_t<F, BindingsRef>
		
	{
		// bind doesn't take anything by reference, so we just straight-up store
		// by value. this will still perform construct-by-reference (rvalue/lvalue)
		using function_t = std::decay_t<F>;
		using bindings_t = tuple_map_t<std::decay, BindingsRef>;

		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(std::forward<FF>(fn))
			, bindings_(std::forward<BB>(bindings))
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args) -> decltype(call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...)))
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> function_t const& { return fn_; }
		auto bindings() const -> bindings_t const& { return bindings_; }

	private:
		function_t fn_;
		bindings_t bindings_;
	};
#endif

#if 0
	template <typename PreF, typename PreBindings, typename NewBindings>
	struct bind_t<bind_t<PreF, PreBindings>, NewBindings>
	{
		using function_t = std::decay_t<PreF>;
		using bindings_t = tuple_map_t<std::decay, bound_arguments_t<PreBindings, NewBindings>>;

		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(fn.fn())
			, bindings_(
				std::forward<decltype(bind_arguments(fn.bindings(), std::forward<BB>(bindings)))>(
					bind_arguments(fn.bindings(), std::forward<BB>(bindings))))
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args) -> decltype(call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...)))
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> PreF const& { return fn_; }
		auto bindings() const -> bindings_t const& { return bindings_; }

	private:
		function_t fn_;
		bindings_t bindings_;
	};
#endif

	// regular bind
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
		-> bind_t<std::remove_reference_t<F>, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}

	template <typename F, typename Params>
	struct result_of<atma::bind_t<F, Params>>
	{
		using type = result_of_t<F>;
	};

#if 0
	template <typename, typename> struct bind_args_tx;
	template <typename, typename, typename> struct bind_args_ii_tx;
	template <typename, typename, uint> struct bind_args_iii_tx;


	template <typename Args, int E, typename... Rest>
	struct bind_args_ii_tx<Args, std::tuple<placeholder_t<E>, Rest...>, E>
	{
		using type = std::tuple< tuple_get_t<E, Args> >;
	};

	template <typename Args, int E, typename... Rest>
	struct bind_args_ii_tx<Args, std::tuple<Rest...>, E>
	{
		using type = std::tuple< tuple_get_t<E, Args> >;
	};

	template <typename Args, int E>
	struct bind_args_ii_tx<Args, std::tuple<>, E>
	{
		using type = std::tuple<>;
	};

	template <typename Args, typename Bindings, uint... idxs>
	struct bind_args_tx<Args, Bindings, idxs_t<idxs...>>
	{
		using type = tuple_cat_t< typename bind_args_iii_tx<Args, Bindings, idxs>::type... >;
	};

	
	template <typename Args, typename Bindings>
	struct bind_args_tx
		: bind_args_ii_tx<Args, Bindings, idxs_list_t<std::tuple_size<Bindings>::value>>
	{
	};

#endif


	template <typename, size_t, typename> struct comb;

	template <typename A, size_t N>
	struct comb<A, N, std::tuple<>>
	{
		using type = std::tuple<>;
	};

	// 
	template <typename ArgsTuple, size_t N, int E, typename... Rest>
	struct comb<ArgsTuple, N, std::tuple<placeholder_t<E>, Rest...>>
	{
		// find placeholder<N> and 
		using type =
			tuple_push_front_t<
				typename comb<ArgsTuple, N + 1, std::tuple<Rest...>>::type,
				tuple_get_t<N, ArgsTuple>
			>;
	};

	// non-placeholder, doesn't contribute to arguments
	template <typename ArgsTuple, size_t N, typename L, typename... Rest>
	struct comb<ArgsTuple, N, std::tuple<L, Rest...>>
	{
		using type =
			typename comb<ArgsTuple, N, std::tuple<Rest...>>::type;
	};

	
	template <typename F, typename Bindings>
	struct function_traits<bind_t<F, Bindings>>
	{
		using result_type = typename function_traits<F>::result_type;
		using tupled_args_type = typename comb<typename function_traits<F>::tupled_args_type, 0, Bindings>::type;
		
		//
		static int const arity = function_traits<F>::arity - tuple_nonplaceholder_size_t<Bindings>::value;
		//
		//
		//template <size_t i>
		//using arg_type = typename function_traits<F>::template arg_type<i + tuple_nonplaceholder_size_t<Bindings>::value>;
	};


	//
	//  curry
	//  ------
	//    takes a function, and a list of bindings, and returns a functor taking those bindings
	//    and any additional implicit bindings
	//
	//
	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings)
		-> bind_t<F, curried_bindings_t<F, Bindings...>>
	{
		auto const param_size = function_traits<F>::arity + (size_t)function_traits<F>::is_memfnptr;
		auto const binding_size = sizeof...(Bindings);
		auto const rem_args = tuple_placeholder_list_t<param_size - binding_size>();

		auto merged_bindings = std::tuple_cat(std::make_tuple(std::forward<Bindings>(bindings)...), rem_args);

		return {std::forward<F>(f), std::forward<decltype(merged_bindings)>(merged_bindings)};
	}




	//
	//  flip
	//  ------
	//    takes a function and generates a binding that flips the order of the arguments
	//
	template <typename F>
	inline auto flip(F&& f)
		-> bind_t<F, tuple_flip_t<curried_bindings_t<F>>>
	{
		return {f, tuple_flip_t<curried_bindings_t<F>>()};
	}

}


