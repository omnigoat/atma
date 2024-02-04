#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename... Fwds> struct functor_list_fwds_t {};
	
}

// Fwds: the forwards, Gs: "previously seen" functors, F: "functor in question", Rs: "yet to evaluate"

namespace atma::detail
{
	template <typename Fwds, typename Gs, typename F>
	struct functor_delegate_;

	template <typename... Fwds, typename... Gs, typename F>
	struct functor_delegate_<functor_list_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...) && std::is_invocable_v<F, Fwds..., Args...>
		constexpr auto operator ()(Args&&... args) const -> decltype(auto)
		{
			return F{}(Fwds{}..., std::forward<Args>(args)...);
		}
	};
}

namespace atma::detail
{
	template <typename Fwds, typename Gs, typename Rs>
	struct functor_list_
	{};

	template <typename Fwds, typename... Gs, typename F, typename... Rs>
	struct functor_list_<Fwds, std::tuple<Gs...>, std::tuple<F, Rs...>>
		: functor_list_<Fwds, std::tuple<Gs..., F>, std::tuple<Rs...>>
		, functor_delegate_<Fwds, std::tuple<Gs...>, F>
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
		: detail::functor_list_<functor_list_fwds_t<Fwds...>, std::tuple<>, std::tuple<Fs...>>
	{
		static_assert((std::is_empty_v<Fwds> && ...),
			"all forwarded functors must be empty functors");

		constexpr functor_list_t() noexcept = default;

		template <typename... Gs>
		constexpr functor_list_t(Gs&&...) noexcept
		{}
	};

	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_list_fwds_t<Fwds...>, Fs&&...)
		-> functor_list_t<functor_list_fwds_t<Fwds...>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_fwds_t<>, std::remove_reference_t<Fs>...>;
}
