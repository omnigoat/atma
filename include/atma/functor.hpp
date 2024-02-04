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
	template <typename...> struct functor_list_stateful_fwds_t {};

	template <typename Fwds, typename Gs, typename F>
	struct functor_delegate_;

	template <typename... Fwds, typename... Gs, typename F>
	struct functor_delegate_<functor_list_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...) && std::is_invocable_v<F, Fwds..., Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(Fwds{}..., std::forward<Args>(args)...);
		}
	};

	template <typename... FwdRefs, typename... Gs, typename F>
	struct functor_delegate_<functor_list_stateful_fwds_t<FwdRefs...>, std::tuple<Gs...>, F>
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
	// case when all forwarded-functors are empty
	template <typename Fwds, bool Empty, typename Fs>
	struct functor_list2_t
		: detail::functor_list_<Fwds, std::tuple<>, Fs>
	{};

	template <typename... Fwds, typename Fs>
	struct functor_list2_t<functor_list_fwds_t<Fwds...>, false, Fs>
		: detail::functor_list_<detail::functor_list_stateful_fwds_t<Fwds...>, std::tuple<>, Fs>
	{
		template <typename... Args>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			using base_type = detail::functor_list_<detail::functor_list_stateful_fwds_t<Fwds...>, std::tuple<>, Fs>;

			return std::apply([&]<class... Ts>(Ts&&... ts) {
				return std::invoke(static_cast<base_type const&>(*this), std::forward<Ts>(ts)..., std::forward<Args>(args)...);
			}, fwds_);
		}

		template <size_t Idx>
		decltype(auto) forwarded_functor() const
			{ return std::get<Idx>(fwds_); }

		auto forwarded_functors() const -> std::tuple<Fwds...> const&
			{ return fwds_; }

	private:
		mutable std::tuple<Fwds...> fwds_;
	};

}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t;

	template <typename... Fwds, typename... Fs>
	struct functor_list_t<functor_list_fwds_t<Fwds...>, Fs...>
		: functor_list2_t<functor_list_fwds_t<std::add_lvalue_reference_t<Fwds>...>, (std::is_empty_v<Fwds> && ...), std::tuple<Fs...>>
	{
		constexpr functor_list_t() noexcept = default;
		constexpr functor_list_t(auto&&...) noexcept {}
	};

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: functor_list2_t<functor_list_fwds_t<>, true, std::tuple<Fs...>>
	{};

	// deduction guides
	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_list_fwds_t<Fwds...>, Fs&&...)
		-> functor_list_t<functor_list_fwds_t<Fwds...>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_fwds_t<>, std::remove_reference_t<Fs>...>;
}
