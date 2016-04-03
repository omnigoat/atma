#pragma once


#include <atma/tuple.hpp>
#include <atma/placeholders.hpp>
#include <atma/function_traits.hpp>
#include <atma/function_composition.hpp>
#include <atma/call_fn.hpp>

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

		template <typename F, typename... B>
		using curried_bindings_t = typename detail::curried_bindings_tx<F, B...>::type;
	}


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
	//  bind_fn_args_t
	//  ----------------
	//    member-function-pointers (but not functors) add an additional argument
	//    whilst resolving concrete bindings, because the first parameter is the
	//    class instance.
	//
	namespace detail
	{
		template <typename F, bool = function_traits<F>::is_memfnptr>
		struct bind_fn_args_tx
		{
			using type = typename function_traits<F>::tupled_args_type;
		};

		template <typename F>
		struct bind_fn_args_tx<F, true>
		{
			using type = 
				tuple_push_front_t<
					typename function_traits<F>::tupled_args_type,
					typename function_traits<F>::class_type*>;
		};

		template <typename F>
		using bind_fn_args_t = typename bind_fn_args_tx<std::decay_t<F>>::type;
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
	inline decltype(auto) call_fn_bound_tuple(F&& f, Bindings&& b, Args&& a)
	{
		auto const binding_size = std::tuple_size<std::decay_t<Bindings>>::value;

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
#if 0
	template <typename F, typename BindingsRef, typename Args>
	struct bind_base_t;

	


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



#else
	namespace detail
	{
		template <typename F, typename BindingsRef, typename Args>
		struct bind_iii_t;

		template <typename F, typename BindingsRef, typename... Args>
		struct bind_iii_t<F, BindingsRef, std::tuple<Args...>>
		{
			// bind doesn't take anything by reference, so we just straight-up store
			// by value. this will still perform construct-by-reference (rvalue/lvalue)
			using function_t = std::decay_t<F>;
			using bindings_t = tuple_map_t<std::decay, BindingsRef>;

			template <typename FF, typename BB>
			bind_iii_t(FF&& fn, BB&& bindings)
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

		template <typename F, typename BindingsRef, bool Op>
		struct bind_ii_t : 
			bind_iii_t<F, BindingsRef, detail::resultant_args_t<detail::bind_fn_args_t<F>, BindingsRef>>
		{
			template <typename FF, typename BB>
			bind_ii_t(FF&& f, BB&& b)
				: bind_iii_t{std::forward<FF>(f), std::forward<BB>(b)}
			{}
		};

		template <typename F, typename BindingsRef>
		struct bind_ii_t<F, BindingsRef, false>
		{
			// bind doesn't take anything by reference, so we just straight-up store
			// by value. this will still perform construct-by-reference (rvalue/lvalue)
			using function_t = std::decay_t<F>;
			using bindings_t = tuple_map_t<std::decay, BindingsRef>;

			template <typename FF, typename BB>
			bind_ii_t(FF&& fn, BB&& bindings)
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
	}

	template <typename F, typename BindingsRef>
	struct bind_t : detail::bind_ii_t<F, BindingsRef, is_callable_v<F>>
	{
		template <typename FF, typename BB>
		bind_t(FF&& f, BB&& b)
			: bind_ii_t{std::forward<FF>(f), std::forward<BB>(b)}
		{}
	};

#if 0
	template <typename PreF, typename PreBindings, typename NewBindings>
	struct bind_t<bind_t<PreF, PreBindings>, NewBindings>
	{
		using function_t = std::decay_t<PreF>;
		using bindings_t = tuple_map_t<std::decay, detail::bound_arguments_t<PreBindings, NewBindings>>;

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
#endif

	//
	//  function_traits
	//  -----------------
	//    specialization for function_traits
	//
	template <typename F, typename Bindings>
	struct function_traits_override<bind_t<F, Bindings>>
	{
		using result_type = typename function_traits<F>::result_type;
		using tupled_args_type = detail::resultant_args_t<detail::bind_fn_args_t<F>, Bindings>;
		template <int Idx> using arg_type = std::tuple_element_t<Idx, tupled_args_type>;

		static bool const is_memfnptr = false;

		constexpr static size_t arity = std::tuple_size_v<tupled_args_type>;
	};




	//
	//  bind
	//  ------
	//    hooray!
	//
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings) -> bind_t<std::remove_reference_t<F>, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}


	//
	//  curry
	//  -------
	//    takes a function, and a list of bindings, and returns a functor taking those bindings
	//    and any additional implicit bindings.
	//
	//    NOTE: this *does* require function_traits<F> to know the number of arguments, which
	//          means you can't curry a templated function/functor. deal with it.
	//
	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings) -> bind_t<F, detail::curried_bindings_t<F, Bindings...>>
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
	inline auto flip(F&& f) -> bind_t<F, tuple_flip_t<detail::curried_bindings_t<F>>>
	{
		return {f, tuple_flip_t<curried_bindings_t<F>>()};
	}

}


