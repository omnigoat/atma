#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename... Fwds>
	struct functor_list_fwds_t
	{
		constexpr functor_list_fwds_t() = default;

		template <typename... FwdsFwd>
		constexpr functor_list_fwds_t(FwdsFwd&&... fwds)
			: fwds{std::forward<FwdsFwd>(fwds)...}
		{}

		std::tuple<Fwds...> fwds;
	};

	template <>
	struct functor_list_fwds_t<>
	{
		constexpr functor_list_fwds_t() = default;
	};
}

// Fwds: the forwards, Gs: "previously seen" functors, F: "functor in question", Rs: "yet to evaluate"
namespace atma::detail
{
	template <typename Fwds, typename Gs, typename F>
	struct functor_delegate_;

	template <typename... Gs, typename F>
	struct functor_delegate_<functor_list_fwds_t<>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Args...> && ...) && std::is_invocable_v<F, Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(std::forward<Args>(args)...);
		}
	};

	template <typename... FwdRefs, typename... Gs, typename F>
	struct functor_delegate_<functor_list_fwds_t<FwdRefs...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, FwdRefs..., Args...> && ...) && std::is_invocable_v<F, FwdRefs..., Args...>
		constexpr decltype(auto) operator ()(FwdRefs... fwds, Args&&... args) const
		{
			return F{}(fwds..., std::forward<Args>(args)...);
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

namespace atma::detail
{
	template <typename Fwds, typename Fs> struct functor_list2_t;

	// case when all forwarded-functors are empty
	template <typename Fs>
	struct functor_list2_t<functor_list_fwds_t<>, Fs>
		: detail::functor_list_<functor_list_fwds_t<>, std::tuple<>, Fs>
	{
		constexpr functor_list2_t(auto&&...) noexcept
		{}
	};

	template <typename... Fwds, typename Fs>
	struct functor_list2_t<functor_list_fwds_t<Fwds...>, Fs>
		: protected detail::functor_list_<functor_list_fwds_t<Fwds&...>, std::tuple<>, Fs>
	{
		using base_type = detail::functor_list_<functor_list_fwds_t<Fwds&...>, std::tuple<>, Fs>;

		constexpr functor_list2_t(functor_list_fwds_t<Fwds...> fwds, auto&&...) noexcept
			: fwds{std::move(fwds.fwds)}
		{}

		template <typename... Args>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return std::apply([&](std::add_lvalue_reference_t<Fwds>... fwds) {
				return std::invoke(static_cast<base_type const&>(*this), fwds..., std::forward<Args>(args)...);
			}, fwds);
		}

		mutable std::tuple<Fwds...> fwds;
	};
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t;

	template <typename... Fwds, typename... Fs>
	struct functor_list_t<functor_list_fwds_t<Fwds...>, Fs...>
		: detail::functor_list2_t<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>
	{
		using base_type = detail::functor_list2_t<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>;
		using base_type::base_type;
	};

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: functor_list_t<functor_list_fwds_t<>, Fs...>
	{};

	// deduction guides
	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_list_fwds_t<Fwds...>, Fs&&...)
		-> functor_list_t<functor_list_fwds_t<Fwds...>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_fwds_t<>, std::remove_reference_t<Fs>...>;
}
