#pragma once

#include <tuple>


namespace atma {

	template <typename F>
	struct function_traits;




	// remove references
	template <typename F>
	struct function_traits<F&>
		: function_traits<F>
	{};

	template <typename F>
	struct function_traits<F&&>
		: function_traits<F>
	{};


	// presumed "callable"
	template <typename T>
	struct function_traits
		: function_traits<decltype(&T::operator())>
	{};


	// function-pointer
	template <typename R, typename... Args>
	struct function_traits<R(*)(Args...)>
		: function_traits<R(Args...)>
	{};


	// member-function-pointer
	template <typename C, typename R, typename... Args>
	struct function_traits<R(C::*)(Args...) const>
		: function_traits<R(Args...)>
	{};

	template <typename C, typename R, typename... Args>
	struct function_traits<R(C::*)(Args...)>
		: function_traits<R(Args...)>
	{};




	template <typename R, typename... Args>
	struct function_traits<R(Args...)>
	{
		using result_type      = R;
		using tupled_args_type = std::tuple<Args...>;

		template <size_t i>
		using arg_type = typename std::tuple_element<i, std::tuple<Args...>>::type;

		static size_t const arity = sizeof...(Args);
	};

}
