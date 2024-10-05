#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename... Fwds>
	struct functor_list_fwds_t
	{};
}

namespace atma::detail::_functor_list_
{
	template <typename Fwds, typename Gs, typename F>
	struct functor_delegate_t;

	template <typename... Fwds, typename... Gs, typename F>
	struct functor_delegate_t<functor_list_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...) && std::is_invocable_v<F, Fwds..., Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(Fwds{}..., std::forward<Args>(args)...);
		}
	};
}

namespace atma::detail::_functor_list_
{
	template <typename Fwds, typename Gs, typename Rs>
	struct functor_chain_t
	{};

	template <typename Fwds, typename... Gs, typename F, typename... Rs>
	struct functor_chain_t<Fwds, std::tuple<Gs...>, std::tuple<F, Rs...>>
		: functor_chain_t<Fwds, std::tuple<Gs..., F>, std::tuple<Rs...>>
		, functor_delegate_t<Fwds, std::tuple<Gs...>, F>
	{};
}

namespace atma::detail::_functor_list_
{
	template <typename Fwds, typename Fs>
	struct chain_selector_t;

	template <typename Fs>
	struct chain_selector_t<functor_list_fwds_t<>, Fs>
		: functor_chain_t<functor_list_fwds_t<>, std::tuple<>, Fs>
	{};

	template <typename... Fwds, typename... Fs>
	struct chain_selector_t<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>
		: functor_chain_t<functor_list_fwds_t<Fwds...>, std::tuple<>, std::tuple<Fs...>>
	{};
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t;

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: functor_list_t<functor_list_fwds_t<>, Fs...>
	{};

	template <typename... Fwds, typename... Fs>
	struct functor_list_t<functor_list_fwds_t<Fwds...>, Fs...>
		: detail::_functor_list_::chain_selector_t<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>
	{
		static_assert((std::is_empty_v<Fwds> && ...),
			"all forwarded types must be empty");

		static_assert((std::is_empty_v<Fs> && ...),
			"all forwarded functors must be empty");

		constexpr functor_list_t(auto&&...) noexcept
		{}
	};

	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_list_fwds_t<Fwds...>, Fs&&...)
		-> functor_list_t<functor_list_fwds_t<Fwds...>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_fwds_t<>, std::remove_reference_t<Fs>...>;
}
