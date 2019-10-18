#pragma once

#include <type_traits>


#define RETURN_TYPE_IF(type, ...) \
	std::enable_if_t<::atma::concepts::detail::all_true_v<__VA_ARGS__>, type>


// functor-call
namespace atma::detail
{
	template <typename F, typename... Fs>
	struct functor_call_
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Args...> && ...)>>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Args...>
		{
			return std::invoke(reinterpret_cast<F const&>(const_cast<functor_call_&>(*this)), std::forward<Args>(args)...);
		}
	};
}

// multi-functor
namespace atma::detail
{
	template <typename, typename>
	struct multi_functor_;

	template <typename... Gs>
	struct multi_functor_<meta::list<Gs...>, meta::list<>>
	{};

	template <typename... Gs, typename F, typename... Fs>
	struct multi_functor_<meta::list<Gs...>, meta::list<F, Fs...>>
		: multi_functor_<meta::list<Gs..., F>, meta::list<Fs...>>
		, functor_call_<F, Gs...>
	{};
}

namespace atma
{
	template <typename... Fs>
	struct multi_functor_t
		: detail::multi_functor_<meta::list<>, meta::list<rmref_t<Fs>...>>
	{
		constexpr multi_functor_t(Fs&& ... fs)
		{}
	};

	template <typename... Fs>
	multi_functor_t(Fs&& ...) -> multi_functor_t<Fs...>;
}


