#pragma once

#include <atma/concepts.hpp>

#include <functional>


//======================================================================
//  is_range_v
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

	struct range_concept
	{
		template <typename T>
		auto contract() -> concepts::specifies<
			SPECIFIES_EXPR(std::begin(std::declval<T>())),
			SPECIFIES_EXPR(std::end(std::declval<T>()))
		>;
	};
}

// range_function_invoke
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

// ranges::for_each
namespace atma
{
	template <typename F>
	struct for_each_fn
	{
		template <typename FF>
		for_each_fn(FF&& f)
			: f_{std::forward<FF>(f)}
		{}

		template <typename R>
		void operator ()(R&& range) const
		{
			for (auto&& x : std::forward<R>(range))
				std::invoke(f_, std::forward<decltype(x)>(x));
		}

	private:
		F f_;
	};

	template <typename R, typename F,
		CONCEPT_MODELS_(range_concept, remove_cvref_t<R>)>
	inline auto operator | (R&& range, for_each_fn<F> const& f) -> void
	{
		f(std::forward<R>(range));
	}

	template <typename F>
	inline auto for_each(F&& f)
	{
		return for_each_fn<F>{std::forward<F>(f)};
	}
}


