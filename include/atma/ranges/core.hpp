#pragma once

#include <functional>


//======================================================================
// atma  detail  is_range_v
//======================================================================
namespace atma
{
	namespace detail
	{
		template <typename T, typename = std::void_t<>>
		struct is_range
		{
			static constexpr bool value = false;
		};

		template <typename T>
		struct is_range<T, std::void_t<
			decltype(std::begin(std::declval<T>())),
			decltype(std::end(std::declval<T>()))>>
		{
			static constexpr bool value = true;
		};
	}

	template <typename T>
	inline constexpr bool is_range_v = detail::is_range<T>::value;
}

namespace atma
{
	namespace detail
	{
		template <typename F, typename Arg>
		decltype(auto) range_function_invoke_impl(F&& f, Arg&& arg)
		{
			return std::invoke(std::forward<F>(f), std::forward<Arg>(arg));
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...> const& tuple) {
			return std::apply(std::forward<F>(f), tuple);
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...>& tuple) {
			return std::apply(std::forward<F>(f), tuple);
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...>&& tuple) {
			return std::apply(std::forward<F>(f), std::move(tuple));
		}
	}

	template <typename F, typename Arg>
	decltype(auto) range_function_invoke(F&& f, Arg&& arg)
	{
		return detail::range_function_invoke_impl(std::forward<F>(f), std::forward<Arg>(arg));
	}
}
