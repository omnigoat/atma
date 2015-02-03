#pragma once

#include <tuple>


namespace atma { namespace xtm {

	template <typename T>
	struct function_traits
		: function_traits<decltype(&T::operator())>
	{};

	template <typename T>
	struct function_traits<T&>
		: function_traits<T>
	{};

	template <typename T>
	struct function_traits<T&&>
		: function_traits<T>
	{};

	template <typename T>
	struct function_traits<T*>
		: function_traits<T>
	{};

	template <typename R, typename... Args>
	struct function_traits<R(&)(Args...)>
		: function_traits<R(*)(Args...)>
	{};




	template <typename C, typename R, typename... Params>
	struct function_traits<R(C::*)(Params...) const>
	{
		using result_type      = R;
		using tupled_args_type = std::tuple<Params...>;
		
		template <size_t i>
		using arg_type = typename std::tuple_element<i, std::tuple<Params...>>::type;

		static size_t const arity = sizeof...(Params);
	};

	template <typename C, typename R, typename... Params>
	struct function_traits<R(C::*)(Params...)>
	{
		using result_type      = R;
		using tupled_args_type = std::tuple<Params...>;

		template <size_t i>
		using arg_type = typename std::tuple_element<i, std::tuple<Params...>>::type;

		static size_t const arity = sizeof...(Params);
	};

	template <typename R, typename... Params>
	struct function_traits<R(*)(Params...)>
	{
		using result_type      = R;
		using tupled_args_type = std::tuple<Params...>;

		template <size_t i>
		using arg_type = typename std::tuple_element<i, std::tuple<Params...>>::type;

		static size_t const arity = sizeof...(Params);
	};

} }
