#pragma once


#include <atma/xtm/tuple.hpp>
#include <atma/xtm/placeholders.hpp>


namespace atma { namespace xtm {

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
			static const size_t fn_arity      = function_traits<F>::arity;
			static const bool   fn_ismemfn    = std::is_member_function_pointer<F>::value;
			static const size_t bindings_size = sizeof...(B);

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




	template <typename R, typename from, typename to>
	struct type_if_castible :
		std::enable_if<std::is_convertible<std::remove_reference_t<from>&, std::remove_reference_t<to>&>::value, R>
	{
	};




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
	auto call_fn(F&& f, Args&&... args) -> typename xtm::function_traits<F>::result_type
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
	auto call_fn(R(C::*fn)(Params...) const, C* c, Args&&... args) -> R
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
		inline auto call_fn_tuple_impl(F&& f, Tuple&& xs, idxs_t<Idxs...>)
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
		{
			return call_fn(
				std::forward<F>(f),
				std::get<Idxs>(std::forward<Tuple>(xs))...);
		}
	}

	// catch-all
	template <typename F, typename Tuple>
	inline auto call_fn_tuple(F&& f, Tuple&& xs)
		-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;

		return detail::call_fn_tuple_impl(
			std::forward<F>(f),
			std::forward<Tuple>(xs),
			idxs_list_t<tuple_size>());
	}

	// memfnptr helpers
	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...), CC* c, Tuple&& xs)
		-> typename type_if_castible<R, CC, C>::type
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC* c, Tuple&& xs)
		-> typename type_if_castible<R, CC, C const>::type
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...), CC&& c, Tuple&& xs)
		-> typename type_if_castible<R, CC, C>::type
	{
		auto const tuple_size = std::tuple_size<std::decay_t<Tuple>>::value;
		static_assert(tuple_size == sizeof...(Params), "incorrect number of arguments");

		return detail::call_fn_tuple_impl(
			f,
			tuple_push_front(std::forward<Tuple>(xs), c),
			idxs_list_t<tuple_size + 1>());
	}

	template <typename R, typename C, typename... Params, typename CC, typename Tuple>
	inline auto call_fn_tuple(R(C::*f)(Params...) const, CC&& c, Tuple&& xs)
		-> typename type_if_castible<R, CC, C const>::type
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
			-> typename atma::xtm::function_traits<typename std::decay<F>::type>::result_type
		{
			return call_fn(
				std::forward<F>(f),
				select_bound_arg(std::get<Idxs>(std::forward<Bindings>(bindings)), std::forward<Args>(args))...);
		}
	}

	template <typename F, typename Bindings, typename Args>
	inline auto call_fn_bound_tuple(F&& f, Bindings&& b, Args&& a)
		-> typename function_traits<std::decay_t<F>>::result_type
	{
		auto const param_size = function_traits<F>::arity + (int)std::is_member_function_pointer<std::decay_t<F>>::value;
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
	template <typename F, typename BindingsRef>
	struct bind_t
	{
		// bind doesn't take anything by reference, so we just straight-up store
		// by value. this will still perform construct-by-reference (rvalue/lvalue)
		using Bindings = tuple_map_t<std::remove_reference, BindingsRef>;

		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(std::forward<FF>(fn)), bindings_(std::forward<BB>(bindings))
		{
		}

		template <typename... Args>
		auto operator ()(Args&&... args)
			-> typename function_traits<F>::result_type
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> F const& { return fn_; }
		auto bindings() const -> Bindings const& { return bindings_; }

	private:
		F fn_;
		Bindings bindings_;
	};

	template <typename PreF, typename PreBindings, typename NewBindings>
	struct bind_t<bind_t<PreF, PreBindings>, NewBindings>
	{
		using Bindings = tuple_map_t<std::remove_reference, bound_arguments_t<PreBindings, NewBindings>>;

		template <typename FF, typename BB>
		bind_t(FF&& fn, BB&& bindings)
			: fn_(fn.fn())
			, bindings_(
				std::forward<decltype(bind_arguments(fn.bindings(), std::forward<BB>(bindings)))>(
					bind_arguments(fn.bindings(), std::forward<BB>(bindings))))
		{
			auto&& blam = bind_arguments(fn.bindings(), std::forward<BB>(bindings));
			bindings_ = blam;
		}

		template <typename... Args>
		auto operator ()(Args&&... args)
			-> typename function_traits<PreF>::result_type
		{
			return call_fn_bound_tuple(fn_, bindings_, std::forward_as_tuple(args...));
		}

		auto fn() const -> PreF const& { return fn_; }
		auto bindings() const -> Bindings const& { return bindings_; }

	private:
		PreF fn_;
		Bindings bindings_;
	};

	// regular bind
	template <typename F, typename... Bindings>
	inline auto bind(F&& f, Bindings&&... bindings)
		-> bind_t<std::remove_reference_t<F>, std::tuple<Bindings...>>
	{
		return {std::forward<F>(f), std::forward_as_tuple(bindings...)};
	}




	template <typename F, typename Bindings>
	struct function_traits<bind_t<F, Bindings>>
		: function_traits<F>
	{};


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
		auto const param_size = function_traits<F>::arity + (int)std::is_member_function_pointer<F>::value;
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

}}
