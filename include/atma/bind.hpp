#pragma once


#include <atma/tuple.hpp>
#include <atma/placeholders.hpp>
#include <atma/function_traits.hpp>
#include <atma/function_composition.hpp>

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
		constexpr inline auto select_bound_arg(Binding&& b, Args&&)
			-> decltype(b)
		{
			return std::forward<Binding>(b);
		}

		template <size_t I, typename Args>
		constexpr inline auto select_bound_arg(placeholder_t<I>, Args&& args)
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
	}


	//
	//  normalize_placeholders_t
	//  --------------------------
	//    sometimes the type of the placeholders in our bindings list is weird, like
	//    `placeholder_t<0> const`, or `placeholder_t<0> const&`. this normalizes them
	//
	namespace detail
	{
		template <typename Binding>
		struct normalize_placeholder_tx
		{
			using type = Binding;
		};

		template <int I>
		struct normalize_placeholder_tx<placeholder_t<I> const> {
			using type = placeholder_t<I>;
		};

		template <int I>
		struct normalize_placeholder_tx<placeholder_t<I> const&> {
			using type = placeholder_t<I>;
		};

		template <typename Bindings>
		using normalize_placeholders_t = tuple_map_t<normalize_placeholder_tx, Bindings>;
	}


	//
	//  highest_placeholder_v
	//  -----------------------
	//    the index of the 'highest' placeholder in our list. returns -1 for no placeholders.
	//
	namespace detail
	{
		template <int Mask, typename Bindings, typename PBindings> struct valid_bindings_tx;

		template <int M, typename B, typename X, typename... R>
		struct valid_bindings_tx<M, B, std::tuple<X, R...>> {
			using type = typename valid_bindings_tx<M, B, std::tuple<R...>>::type;
		};

		template <int M, typename B, int Idx, typename... R>
		struct valid_bindings_tx<M, B, std::tuple<placeholder_t<Idx>, R...>> {
			using type = typename valid_bindings_tx<M | (1 << Idx), B, std::tuple<R...>>::type;
		};

		template <int M, typename B>
		struct valid_bindings_tx<M, B, std::tuple<>>
		{
			// this checks that we have a contiguous list of placeholders. currently checks up to 8 bindings.
			static_assert(M == 0x0 || M == 0x1 || M == 0x3 || M == 0x7 || M == 0xf || M == 0x1f || M == 0x3f || M == 0x7f, "invalid bindings");
			using type = B;
		};

		template <typename Bindings>
		using valid_bindings_t = typename valid_bindings_tx<0, Bindings, normalize_placeholders_t<Bindings>>::type;
	}


	//
	//  highest_placeholder_v
	//  -----------------------
	//    the index of the 'highest' placeholder in our list. returns -1 for no placeholders.
	//
	namespace detail
	{
		template <typename Bindings> struct highest_placeholder_tx;
		template <typename Bindings> constexpr int const highest_placeholder_v = highest_placeholder_tx<normalize_placeholders_t<Bindings>>::value;

		template <int I, typename... Bindings>
		struct highest_placeholder_tx<std::tuple<placeholder_t<I>, Bindings...>> {
			static int const value = I > highest_placeholder_v<std::tuple<Bindings...>> ? I : highest_placeholder_tx<std::tuple<Bindings...>>::value;
		};

		template <typename X, typename... Bindings>
		struct highest_placeholder_tx<std::tuple<X, Bindings...>>
		{
			static int const value = highest_placeholder_v<std::tuple<Bindings...>>;
		};

		template <>
		struct highest_placeholder_tx<std::tuple<>>
		{
			static int const value = -1;
		};
	}


	//
	//  original_args_type_t
	//  ----------------------
	//    given an index of an incoming argument (so funtor called with two arguments, indexes 0
	//    and 1), find out which argument it maps to from the original function. this would fail
	//    if given an index for a placeholder that doesn't exist in this binding.
	//
	namespace detail
	{
		template <typename Args, typename Bindings, int Bidx, int Idx>
		struct original_args_type_t;

		template <typename Args, int Bidx, int Idx, typename X, typename... Bs>
		struct original_args_type_t<Args, std::tuple<X, Bs...>, Bidx, Idx> {
			using type = typename original_args_type_t<Args, std::tuple<Bs...>, Bidx + 1, Idx>::type;
		};

		template <typename Args, int Bidx, int Midx, typename... Bs>
		struct original_args_type_t<Args, std::tuple<placeholder_t<Midx>, Bs...>, Bidx, Midx> {
			using type = tuple_get_t<Bidx, Args>;
		};

		template <typename A, int I, int B>
		struct original_args_type_t<A, std::tuple<>, B, I>
		{
			// uh oh!
		};
	}


	//
	//  resultant_args_t
	//  ------------------
	//    takes the arguments of the original function, the bindings given, and returns
	//    a tuple of the parameters of the new bind-functor
	//
	namespace detail
	{
		template <typename Args, typename Bindings, int Idx, int End>
		struct resultant_args_tx
		{
			static_assert(End != 0, "bad End");

			using type = typename tuple_push_front_t<
				typename resultant_args_tx<Args, Bindings, Idx + 1, End>::type,
				typename original_args_type_t<Args, Bindings, 0, Idx>::type>;
		};

		template <typename Args, typename Bindings, int Fin>
		struct resultant_args_tx<Args, Bindings, Fin, Fin>
		{
			using type = std::tuple<>;
		};

		template <typename Args, typename Bindings>
		using resultant_args_t = typename resultant_args_tx<
			Args, normalize_placeholders_t<Bindings>,
			0, highest_placeholder_v<Bindings> + 1>::type;
	}


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
	auto call_fn(F&& f, Args&&... args) -> decltype(auto)
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

		template <typename R, typename from, typename to>
		struct type_if_castible :
			std::enable_if<std::is_convertible<std::remove_reference_t<from>&, std::remove_reference_t<to>&>::value, R>
		{};

		template <typename R, typename From, typename To>
		using type_if_castible_t = typename type_if_castible<R, From, To>::type;
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
	inline auto call_fn_tuple(R(C::*f)(Params...), CC* c, Tuple&& xs) -> detail::type_if_castible_t<R, CC, C>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC* c, Tuple&& xs) -> detail::type_if_castible_t<R, CC, C const>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...), CC&& c, Tuple&& xs) -> detail::type_if_castible_t<R, CC, C>
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC&& c, Tuple&& xs) -> detail::type_if_castible_t<R, CC, C const>
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
		inline decltype(auto) call_fn_bound_tuple_impl(F&& f, Bindings&& bindings, Args&& args, idxs_t<Idxs...>)
		{
			return call_fn(std::forward<F>(f), select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
		}
	}

	template <typename F, typename Bindings, typename Args>
	inline decltype(auto) call_fn_bound_tuple(F&& f, Bindings&& b, Args&& a)
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
#if 1
	template <typename F, typename BindingsRef, typename Args>
	struct bind_base_t;


	template <typename F, typename BindingsRef, typename... Args>
	struct bind_base_t<F, BindingsRef, std::tuple<Args...>>
	{
		// bind doesn't take anything by reference, so we just straight-up store
		// by value. this will still perform construct-by-reference (rvalue/lvalue)
		using function_t = std::decay_t<F>;
		using bindings_t = tuple_map_t<std::decay, BindingsRef>;

		template <typename FF, typename BB>
		bind_base_t(FF&& fn, BB&& bindings)
			: fn_(std::forward<FF>(fn))
			, bindings_(std::forward<BB>(bindings))
		{
		}

		auto operator ()(Args... args) -> decltype(auto)
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> function_t const& { return fn_; }
		auto bindings() const -> bindings_t const& { return bindings_; }

	private:
		function_t fn_;
		bindings_t bindings_;
	};


	template <typename F, typename BindingsRef>
	struct bind_t : 
		bind_base_t<F, detail::valid_bindings_t<BindingsRef>,
			detail::resultant_args_t<
				std::conditional_t<function_traits<F>::is_memfnptr,
					tuple_push_front_t<
						typename function_traits<F>::tupled_args_type,
						typename function_traits<F>::class_type>,
					typename function_traits<F>::tupled_args_type>,
				BindingsRef>>
	{
		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: bind_base_t{std::forward<FF>(fn), std::forward<BB>(bindings)}
		{}
	};


	// I don't think this is working as intended
#if 0
	template <typename PreF, typename PreBindings, typename PreArguments, typename NewBindings>
	struct bind_t<bind_base_t<PreF, PreBindings, PreArguments>, NewBindings>
		: bind_base_t<PreF, detail::bound_arguments_t<PreBindings, detail::valid_bindings_t<NewBindings>>,
			detail::resultant_args_t<
				std::conditional_t<function_traits<PreF>::is_memfnptr,
					tuple_push_front_t<
						typename function_traits<PreF>::tupled_args_type,
						typename function_traits<PreF>::class_type>,
					typename function_traits<PreF>::tupled_args_type>,
				detail::bound_arguments_t<PreBindings, NewBindings>>>
	{
		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: bind_base_t{
				fn_(fn.fn()),
				std::forward<decltype(bind_arguments(fn.bindings(), std::forward<BB>(bindings)))>(
					bind_arguments(fn.bindings(), std::forward<BB>(bindings)))}
		{}
	};
#endif



	template <typename F, typename Bindings>
	struct function_traits<bind_t<F, Bindings>>
	{
		using result_type = typename function_traits<F>::result_type;
		using tupled_args_type = detail::resultant_args_t<typename function_traits<F>::tupled_args_type, Bindings>;
		template <int Idx> using arg_type = typename std::tuple_element<Idx, tupled_args_type>::type;

		static bool const is_memfnptr = false;
		using class_type = void;

		static int const arity = function_traits<F>::arity - tuple_nonplaceholder_size_t<detail::normalize_placeholders_t<Bindings>>::value;
	};


#else
	template <typename F, typename BindingsRef>
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
		auto operator ()(Args&&... args) -> decltype(auto)
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> function_t const& { return fn_; }
		auto bindings() const -> bindings_t const& { return bindings_; }

	private:
		function_t fn_;
		bindings_t bindings_;
	};

	template <typename F, typename Bindings>
	struct function_traits<bind_t<F, Bindings>>
	{
		//using result_type = typename function_traits<F>::result_type;
		//using tupled_args_type = detail::resultant_args_t<typename function_traits<F>::tupled_args_type, Bindings>;
		//template <int Idx> using arg_type = typename std::tuple_element<Idx, tupled_args_type>::type;

		static bool const is_memfnptr = false;
		using class_type = void;

		//static int const arity = function_traits<F>::arity - tuple_nonplaceholder_size_t<detail::normalize_placeholders_t<Bindings>>::value;
	};

#endif




	//
	//  bind
	//  ------
	//    hooray!
	//
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
		-> bind_t<std::remove_reference_t<F>, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}


	//
	//  curry
	//  -------
	//    takes a function, and a list of bindings, and returns a functor taking those bindings
	//    and any additional implicit bindings	
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


