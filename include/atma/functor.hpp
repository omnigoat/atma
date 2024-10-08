#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename... Fwds>
	struct functor_cascade_fwds_t
	{};
}

namespace atma::detail::_functor_cascade_
{
	template <typename Fwds, typename Gs, typename F>
	struct dispatcher_t;

	template <typename... Fwds, typename... Gs, typename F>
	struct dispatcher_t<functor_cascade_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...) && std::is_invocable_v<F, Fwds..., Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(Fwds{}..., std::forward<Args>(args)...);
		}
	};
}

namespace atma::detail::_functor_cascade_
{
	template <typename Fwds, typename Gs, typename Rs>
	struct link_t
	{};

	template <typename Fwds, typename... Gs, typename F, typename... Rs>
	struct link_t<Fwds, std::tuple<Gs...>, std::tuple<F, Rs...>>
		: link_t<Fwds, std::tuple<Gs..., F>, std::tuple<Rs...>>
		, dispatcher_t<Fwds, std::tuple<Gs...>, F>
	{};
}

namespace atma::detail::_functor_cascade_
{
	template <typename Fwds, typename Fs>
	struct chain_selector_t
		: link_t<functor_cascade_fwds_t<>, std::tuple<>, Fs>
	{};

	template <typename... Fwds, typename... Fs>
	struct chain_selector_t<functor_cascade_fwds_t<Fwds...>, std::tuple<Fs...>>
		: link_t<functor_cascade_fwds_t<Fwds...>, std::tuple<>, std::tuple<Fs...>>
	{};
}

namespace atma
{
	template <typename... Fs>
	struct functor_cascade_t
		: functor_cascade_t<functor_cascade_fwds_t<>, Fs...>
	{};

	template <typename... Fwds, typename... Fs>
	struct functor_cascade_t<functor_cascade_fwds_t<Fwds...>, Fs...>
		: detail::_functor_cascade_::chain_selector_t<functor_cascade_fwds_t<Fwds...>, std::tuple<Fs...>>
	{
		static_assert((std::is_empty_v<Fwds> && ...),
			"all forwarded types must be empty");

		static_assert((std::is_empty_v<Fs> && ...),
			"all functors must be empty");

		constexpr functor_cascade_t(auto&&...) noexcept
		{}
	};

	template <typename... Fwds, typename... Fs>
	functor_cascade_t(functor_cascade_fwds_t<Fwds...>, Fs...)
		-> functor_cascade_t<functor_cascade_fwds_t<Fwds...>, Fs...>;
	
	template <typename... Fs>
	functor_cascade_t(Fs...)
		-> functor_cascade_t<functor_cascade_fwds_t<>, Fs...>;
}
