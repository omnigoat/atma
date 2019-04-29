#pragma once

#include "meta.hpp"
#include "types.hpp"

#include <tuple>
#include <functional>


// this is probably the way I want to go in the future for traits
namespace atma
{
	template <size_t N, typename F, typename = std::void_t<>>
	struct has_n_arguments_tx
		: std::false_type
	{};

	template <typename F>
	struct has_n_arguments_tx<0, F, std::void_t<std::invoke_result_t<F>>>
		: std::true_type
	{};

	template <typename F>
	struct has_n_arguments_tx<1, F, std::void_t<std::invoke_result_t<F, meta::any_t>>>
		: std::true_type
	{};

	template <size_t N, typename F>
	inline bool constexpr has_n_arguments_v = has_n_arguments_tx<N, F>::value;

	template <typename R, typename F, typename... Args>
	struct invoke_result_returns_tx
		: std::bool_constant<std::is_same_v<R, std::invoke_result_t<F, Args...>>>
	{};

	template <typename R, typename F, typename... Args>
	inline bool constexpr invoke_result_returns_v = invoke_result_returns_tx<R, F, Args...>::value;

}



namespace atma
{
	//
	//  function_traits
	//  -----------------
	//    good ol' function traits
	//
	namespace detail
	{
		// functor
		template <typename T>
		struct function_traits_tx
			: function_traits_tx<decltype(&T::operator())>
		{
			// subtle interaction: a functor is *not* a member-function-pointer,
			// even though we derive the majority of our definition from the member-
			// function-pointer of its call-operator
			static bool const is_memfnptr = false;
		};

		// function-pointer
		template <typename R, typename... Args>
		struct function_traits_tx<R(*)(Args...)>
			: function_traits_tx<R(Args...)>
		{};

		// member-function-pointer
		template <typename C, typename R, typename... Args>
		struct function_traits_tx<R(C::*)(Args...) const>
			: function_traits_tx<R(Args...)>
		{
			constexpr static bool is_memfnptr = true;
			using class_type = C;
		};

		template <typename C, typename R, typename... Args>
		struct function_traits_tx<R(C::*)(Args...)>
			: function_traits_tx<R(Args...)>
		{
			constexpr static bool is_memfnptr = true;
			using class_type = C;
		};

		// function-type
		template <typename R, typename... Args>
		struct function_traits_tx<R(Args...)>
		{
			using result_type = R;
			using tupled_args_type = std::tuple<Args...>;

			template <size_t i>
			using arg_type = std::tuple_element_t<i, std::tuple<Args...>>;

			constexpr static size_t const arity = sizeof...(Args);
			constexpr static bool   const is_memfnptr = false;
			using class_type = void;
		};
	}

	template <typename T>
	struct function_traits_override
		: detail::function_traits_tx<T>
	{};

	template <typename F>
	using function_traits = function_traits_override<std::decay_t<F>>;
	

}
