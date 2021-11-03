module;

#include <atma/tuple.hpp>
#include <atma/function_composition.hpp>

#include <type_traits>
#include <tuple>
#include <functional>

export module atma.bind;

import atma.types;

export namespace atma
{
	//
	//  placeholder_t
	//  ---------------------------
	//    hooray!
	//
	template <int I>
	struct placeholder_t
	{
		// placeholder_t is an identity function (i.e., it constructs itself)
		using type = placeholder_t<I>;
		static int const value = I;
	};

	//
	//  tuple_placeholder_list_t
	//  tuple_placeholder_range_t
	//  ---------------------------
	//    tuple_placeholder_list_t<2> === std::tuple<placeholder_t<0>, placeholder_t<1>>
	//    tuple_placeholder_range_t<1, 3> === std::tuple<placeholder_t<1>, placeholder_t<2>>
	//
	template <size_t begin, size_t end, int step = 1>
	using tuple_placeholder_range_t =
		tuple_idxs_map_t<placeholder_t, idxs_range_t<begin, end, step>>;

	template <size_t count>
	using tuple_placeholder_list_t =
		tuple_idxs_map_t<placeholder_t, idxs_list_t<count>>;
}

export namespace std
{
	template <int I>
	struct is_placeholder<atma::placeholder_t<I>> : std::integral_constant<int, I + 1>
	{};
}


export constexpr atma::placeholder_t<0> const arg1;
export constexpr atma::placeholder_t<1> const arg2;
export constexpr atma::placeholder_t<2> const arg3;
export constexpr atma::placeholder_t<3> const arg4;
export constexpr atma::placeholder_t<4> const arg5;
export constexpr atma::placeholder_t<5> const arg6;
export constexpr atma::placeholder_t<6> const arg7;
export constexpr atma::placeholder_t<7> const arg8;


export namespace atma
{
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

			using type = decltype(std::tuple_cat(
				std::declval<std::tuple<B...>>(),
				std::declval<tuple_placeholder_list_t<fn_arity + (int)fn_ismemfn - (int)bindings_size>>()));
		};

		template <typename F, typename... B>
		using curried_bindings_t = typename detail::curried_bindings_tx<F, B...>::type;
	}


	//
	//  select_bound_arg
	//  -------------------
	//    chooses the correct binding/argument:
	//
	//      select_bound_arg(2, std::make_tuple(4, 5)) == 2
	//      select_bound_arg(placeholder_t<1>(), std::make_tuple(4, 5)) == 5
	//
	namespace detail
	{
		template <typename Binding, typename Args>
		constexpr inline decltype(auto) select_bound_arg(Binding&& b, Args&&)
		{
			return std::forward<Binding>(b);
		}

		template <size_t I, typename Args>
		constexpr inline decltype(auto) select_bound_arg(placeholder_t<I>, Args&& args)
		{
			return std::get<I>(std::forward<Args>(args));
		}

		template <typename BindExpr, typename Args>
		requires std::is_bind_expression_v<std::decay_t<BindExpr>>
		constexpr inline decltype(auto) select_bound_arg(BindExpr&& bindexpr, Args&& args)
		{
			return std::apply(std::forward<BindExpr>(bindexpr), std::forward<Args>(args));
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
		template <typename Bindings, typename Args, size_t... Idxs>
		inline decltype(auto) bind_arguments_impl(Bindings&& bindings, Args&& args, idxs_t<Idxs...>)
		{
			return std::forward_as_tuple(select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
		}

		template <typename Bindings, typename Args>
		inline decltype(auto) bind_arguments(Bindings&& bindings, Args&& args)
		{
			auto const bindings_count = std::tuple_size<std::remove_reference_t<Bindings>>::value;

			return detail::bind_arguments_impl(
				std::forward<Bindings>(bindings),
				std::forward<Args>(args),
				idxs_list_t<bindings_count>());
		}

		template <typename Bindings, typename Args>
		using bound_arguments_t = decltype(bind_arguments(std::declval<Bindings>(), std::declval<Args>()));
	}


	//
	//  normalizing placeholders
	//  --------------------------
	//  placeholders are forwarded as 'placeholder_t<I> const&', and we don't want
	//  to do our type comparisons against const-reffed types
	//
	namespace detail
	{
		template <typename T>
		struct normalize_placeholder_tx
			{ using type = T; };

		template <typename T>
		requires (std::is_placeholder_v<std::remove_cvref_t<T>> != 0)
		struct normalize_placeholder_tx<T>
			{ using type = std::remove_cvref_t<T>; };

		template <typename T>
		requires (std::is_placeholder_v<std::remove_cvref_t<T>> == 0)
		decltype(auto) normalize_placeholder(T&& t)
			{ return std::forward<T>(t); }

		template <typename T>
		requires (std::is_placeholder_v<std::remove_cvref_t<T>> != 0)
		auto normalize_placeholder(T&& x) -> std::remove_cvref_t<T>
			{ return x; }
	}


	//
	//  forward_as_bindings
	//  ---------------------
	//  
	//
	namespace detail
	{
		template <typename... Bindings>
		inline auto forward_as_bindings(Bindings&&... bindings)
		{
			return std::make_tuple(detail::normalize_placeholder(std::forward<Bindings>(bindings))...);
		}

		template <typename... Bindings>
		using bindings_tuple_t = decltype(forward_as_bindings(std::declval<Bindings>()...));
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
		using valid_bindings_t = typename valid_bindings_tx<0, Bindings, Bindings>::type;
	}


	//
	//  highest_placeholder_v
	//  -----------------------
	//    the index of the 'highest' placeholder in our list. returns -1 for no placeholders.
	//
	namespace detail
	{
		template <typename Bindings> struct highest_placeholder_tx;
		template <typename Bindings> constexpr int highest_placeholder_v = highest_placeholder_tx<Bindings>::value;

		template <int I, typename... Bindings>
		struct highest_placeholder_tx<std::tuple<placeholder_t<I>, Bindings...>> {
			static int const v2 = highest_placeholder_v<std::tuple<Bindings...>>;
			static int const value = (I > v2) ? I : highest_placeholder_tx<std::tuple<Bindings...>>::value;
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
	//  original_args_type_tx
	//  ----------------------
	//    given an index of an incoming argument (so funtor called with two arguments, indexes 0
	//    and 1), find out which argument it maps to from the original function. this would fail
	//    if given an index for a placeholder that doesn't exist in this binding.
	//
	namespace detail
	{
		template <typename Args, typename Bindings, int Bidx, int Idx>
		struct original_args_type_tx;

		template <typename Args, int Bidx, int Idx, typename X, typename... Bs>
		struct original_args_type_tx<Args, std::tuple<X, Bs...>, Bidx, Idx> {
			using type = typename original_args_type_tx<Args, std::tuple<Bs...>, Bidx + 1, Idx>::type;
		};

		template <typename Args, int Bidx, int Midx, typename... Bs>
		struct original_args_type_tx<Args, std::tuple<placeholder_t<Midx>, Bs...>, Bidx, Midx> {
			using type = tuple_get_t<Bidx, Args>;
		};

		template <typename A, int I, int B>
		struct original_args_type_tx<A, std::tuple<>, B, I>
		{
			// will always fail. just a nicer error. we ran out of bindings.
			static_assert(I == B, "invalid index for placeholder");
		};

		template <typename Args, typename Bindings, int BindIdx>
		using original_args_type_t = typename original_args_type_tx<Args, Bindings, 0, BindIdx>::type;

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
				original_args_type_t<Args, Bindings, Idx>>;
		};

		template <typename Args, typename Bindings, int Fin>
		struct resultant_args_tx<Args, Bindings, Fin, Fin>
		{
			using type = std::tuple<>;
		};

		template <typename Args, typename Bindings>
		using resultant_args_t = typename resultant_args_tx<
			Args, Bindings,
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
		template <typename F>
		struct bind_fn_args_tx
		{
			using type = typename function_traits<F>::tupled_args_type;
		};

		template <typename F>
		struct bind_fn_args_tx<F&>
			: bind_fn_args_tx<F>
		{};

		template <typename R, typename C, typename... Args>
		struct bind_fn_args_tx<R (C::*)(Args...)>
		{
			using type = std::tuple<C*, Args...>;
		};

		template <typename R, typename C, typename... Args>
		struct bind_fn_args_tx<R (C::*)(Args...) const>
		{
			using type = std::tuple<C*, Args...>;
		};

		template <typename R, typename C, typename... Args>
		struct bind_fn_args_tx<R (C::*)(Args...)&>
		{
			using type = std::tuple<C*, Args...>;
		};

		template <typename R, typename C, typename... Args>
		struct bind_fn_args_tx<R (C::*)(Args...) const&>
		{
			using type = std::tuple<C*, Args...>;
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
		inline decltype(auto) call_fn_bound_tuple_impl(F&& f, Bindings&& bindings, Args&& args, idxs_t<Idxs...>)
		{
			return std::invoke(std::forward<F>(f), select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
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
	namespace detail
	{
		template <typename F, typename BindingsRef, typename Args>
		struct bind_iii_t;

		template <typename F, typename BindingsRef, typename... Args>
		struct bind_iii_t<F, BindingsRef, std::tuple<Args...>>
		{
			using function_t = std::decay_t<F>;
			using bindings_t = tuple_map_t<std::decay, BindingsRef>;

			template <typename FF, typename BB>
			bind_iii_t(FF&& fn, BB&& bindings)
				: fn_(std::forward<FF>(fn))
				, bindings_(std::forward<BB>(bindings))
			{}

			decltype(auto) operator ()(Args... args) const
			{
				// we must ignore the const-qualifier because reasons
				auto self = const_cast<bind_iii_t<F, BindingsRef, std::tuple<Args...>>*>(this);
				return call_fn_bound_tuple(self->fn_, self->bindings_, std::forward_as_tuple(args...));
			}

			auto fn() const -> function_t const& { return fn_; }
			auto bindings() const -> bindings_t const& { return bindings_; }

		private:
			function_t fn_;
			bindings_t bindings_;
		};

		// bind_ii_t has two forms: one that has a concrete functor-operator, and one that
		// has a variadic-typed one, where the former allows for more composition and such.
		template <typename F, typename BindingsRef, bool ConcreteOperator>
		struct bind_ii_t
			: bind_iii_t<F, BindingsRef, detail::resultant_args_t<detail::bind_fn_args_t<F>, BindingsRef>>
		{
			template <typename FF, typename BB>
			bind_ii_t(FF&& f, BB&& b)
				: bind_iii_t<F, BindingsRef, detail::resultant_args_t<detail::bind_fn_args_t<F>, BindingsRef>>
					{std::forward<FF>(f), std::forward<BB>(b)}
			{}
		};

		template <typename F, typename BindingsRef>
		struct bind_ii_t<F, BindingsRef, false>
		{
			using function_t = std::decay_t<F>;
			using bindings_t = tuple_map_t<std::decay, BindingsRef>;

			template <typename FF, typename BB>
			bind_ii_t(FF&& fn, BB&& bindings)
				: fn_(std::forward<FF>(fn))
				, bindings_(std::forward<BB>(bindings))
			{}

			template <typename... Args>
			decltype(auto) operator ()(Args&&... args) const
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
	struct bind_t
		: detail::bind_ii_t<F, BindingsRef, is_callable_v<F>>
	{
		template <typename FF, typename BB>
		bind_t(FF&& f, BB&& b)
			: detail::bind_ii_t<F, BindingsRef, is_callable_v<F>>
				{std::forward<FF>(f), std::forward<BB>(b)}
		{}
	};

	template <typename PreF, typename PreBindings, typename NewBindings>
	struct bind_t<bind_t<PreF, PreBindings>, NewBindings> 
		: detail::bind_ii_t<PreF, tuple_map_t<std::decay, detail::bound_arguments_t<PreBindings, NewBindings>>, is_callable_v<PreF>>
	{
		template <typename FF, typename BB>
		bind_t(FF&& f, BB&& b)
			: detail::bind_ii_t<PreF, tuple_map_t<std::decay, detail::bound_arguments_t<PreBindings, NewBindings>>, is_callable_v<PreF>>
				{f.fn(), detail::bind_arguments(f.bindings(), std::forward<BB>(b))}
		{}
	};

	template <typename F, typename Bindings>
	bind_t(F&&, Bindings) -> bind_t<std::remove_reference_t<F>, Bindings>;
}

export namespace std
{
	template <typename F, typename... Args>
	struct is_bind_expression<atma::bind_t<F, Args...>> : true_type
	{};
}

export namespace atma
{
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

		constexpr static size_t arity = std::tuple_size_v<tupled_args_type>;
	};

	//
	//  bind
	//  ------
	//    hooray!
	//
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
	{
		constexpr auto bindings_required = (size_t)std::is_member_function_pointer_v<F> + function_traits<rm_ref_t<F>>::arity;
		
		static_assert(sizeof...(bindings) == bindings_required,
			"atma::bind - Incorrect number of arguments supplied. Check the Callable type you "
			"are binding to, and make sure any binding to a member-function-pointer accounts "
			"for the instance argument.");

		return bind_t{std::forward<F>(f), detail::forward_as_bindings(bindings...)};
	}

	//
	//  bind_from
	//  -----------
	//  takes an index for the argument from which we will start binding
	// 
	//  all preceeding arguments are automatically curried
	// 
	//  examples:
	//    bind_from<2>(f, "hello") === bind(f, arg1, arg2, "hello")
	//    bind_from<1>(f, 4) === bind(f, arg1, 4)
	//
	template <size_t Idx, typename F, typename... Bindings>
	inline auto bind_from(F&& f, Bindings&&... bindings)
	{
		constexpr auto bindings_required = (size_t)std::is_member_function_pointer_v<F> +function_traits<rm_ref_t<F>>::arity - Idx;

		static_assert(sizeof...(bindings) == bindings_required,
			"atma::bind_from - Incorrect number of arguments supplied. Check the Callable type you "
			"are binding to, and make sure any binding to a member-function-pointer accounts "
			"for the instance argument, and also from which index you are binding from.");

		auto merged_bindings = std::tuple_cat(
			tuple_placeholder_list_t<Idx>(),
			detail::forward_as_bindings(bindings...));

		return bind_t{std::forward<F>(f), merged_bindings};
	}

	//
	//  curry
	//  -------
	//  takes a function, and a list of bindings, and returns a functor taking those bindings
	//  and any additional implicit bindings.
	//
	//  relies on function_traits
	//
	template <typename F, typename... Bindings>
	inline auto curry(F&& f, Bindings&&... bindings)
	{
		// don't unnecessarily wrap a bind expression with no additional bindings
		if constexpr (std::is_bind_expression_v<rm_ref_t<F>> && sizeof...(Bindings) == 0)
		{
			return std::forward<F>(f);
		}
		else
		{
			auto const class_type_arg_count = (size_t)std::is_member_function_pointer_v<F>;
			auto const trailing_arg_count = class_type_arg_count + function_traits<rm_ref_t<F>>::arity - sizeof...(Bindings);

			auto merged_bindings = std::tuple_cat(
				detail::forward_as_bindings(bindings...),
				tuple_placeholder_list_t<trailing_arg_count>());

			return bind_t{std::forward<F>(f), merged_bindings};
		}
	}


	//
	//  flip
	//  ------
	//    takes a function and generates a binding that flips the order of the arguments
	//
	template <typename F>
	inline auto flip(F&& f) -> bind_t<F, tuple_flip_t<detail::curried_bindings_t<F>>>
	{
		return {f, tuple_flip_t<detail::curried_bindings_t<F>>()};
	}

}


