#pragma once

#include <atma/idxs.hpp>
#include <atma/tuple.hpp>

#include <utility>
#include <type_traits>


namespace atma
{

	//
	//  call_fn
	//  ---------
	//    takes a callable object and a variadic list of arguments, and calls the
	//    object with those arguments. @f may be a function-pointer,
	//    a member-function-pointer (with the first argument being a
	//    pointer/ref to an instantiation of the class), or a generic callable
	//    object, like a std::function<>, or even a lambda.
	//
	//    SERIOUSLY, this can be replaced by std::invoke when c++17 is more prevalent
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


}
