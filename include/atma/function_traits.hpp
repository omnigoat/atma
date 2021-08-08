#pragma once

#include <tuple>
#include <functional>

import atma.types;

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
