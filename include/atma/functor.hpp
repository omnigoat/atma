#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
#if 0
	template <typename... Base>
	struct functor_list_base_t
	{
		constexpr functor_list_base_t() = default;

		constexpr functor_list_base_t(Base... rhs)
			: value{std::forward<Base>(rhs)...}
		{}

		std::tuple<Base...> value;
	};
#else
	template <typename Base = void>
	struct functor_list_base_t
	{
		constexpr functor_list_base_t() = default;

		template <typename BaseFwd>
		constexpr functor_list_base_t(BaseFwd&& rhs)
			: value{std::forward<BaseFwd>(rhs)}
		{}

		Base value;
	};

	template <typename T>
	functor_list_base_t(T&) -> functor_list_base_t<T&>;

	template <typename T>
	functor_list_base_t(T const&&) -> functor_list_base_t<T>;
#endif // 0


	template <>
	struct functor_list_base_t<>
	{
		constexpr functor_list_base_t() = default;
	};
}

// Fwds: the forwards, Gs: "previously seen" functors, F: "functor in question", Rs: "yet to evaluate"
namespace atma::detail
{
	template <typename Base, typename Gs, typename F>
	struct functor_delegate_;

	template <typename... Gs, typename F>
	struct functor_delegate_<functor_list_base_t<>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Args...> && ...) && std::is_invocable_v<F, Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(std::forward<Args>(args)...);
		}
	};

	template <typename Base, typename... Gs, typename F>
	struct functor_delegate_<functor_list_base_t<Base>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Base&, Args...> && ...) && std::is_invocable_v<F, Base&, Args...>
		constexpr decltype(auto) operator ()(Base& base, Args&&... args) const
		{
			return F{}(base, std::forward<Args>(args)...);
		}
	};
}

namespace atma::detail
{
	template <typename Base, typename Gs, typename Rs>
	struct functor_list_;

	template <typename Gs>
	struct functor_list_<functor_list_base_t<>, Gs, std::tuple<>>
	{};

	template <typename Base, typename Gs>
	struct functor_list_<functor_list_base_t<Base>, Gs, std::tuple<>>
		: Base
	{
		template <typename BaseFwd>
		constexpr functor_list_(BaseFwd&& base)
			: Base{std::forward<BaseFwd>(base)}
		{}
	};

	template <typename BaseWrap, typename... Gs, typename F, typename... Rs>
	struct functor_list_<BaseWrap, std::tuple<Gs...>, std::tuple<F, Rs...>>
		: functor_list_<BaseWrap, std::tuple<Gs..., F>, std::tuple<Rs...>>
		, functor_delegate_<BaseWrap, std::tuple<Gs...>, F>
	{};
}

namespace atma::detail
{
	// default case when Base is not present
	template <typename Base, typename Fs>
	struct functor_list2_t;

	template <typename Fs>
	struct functor_list2_t<functor_list_base_t<>, Fs>
		: detail::functor_list_<functor_list_base_t<>, std::tuple<>, Fs>
	{
		constexpr functor_list2_t(auto&&...) noexcept
		{}
	};

	template <typename Base, typename Fs>
	struct functor_list2_t<functor_list_base_t<Base>, Fs>
		: detail::functor_list_<functor_list_base_t<Base>, std::tuple<>, Fs>
	{
		using base_type = detail::functor_list_<functor_list_base_t<Base>, std::tuple<>, Fs>;

		template <typename BaseFwd>
		constexpr functor_list2_t(BaseFwd&& base, auto&&...) noexcept
			: base_type{std::forward<BaseFwd>(base).value}
		{}

		template <typename... Args>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return std::invoke(
				static_cast<base_type const&>(*this),
				const_cast<Base&>(static_cast<Base const&>(*this)),
				std::forward<Args>(args)...);
		}
	};
}

namespace atma
{
	template <typename Base, typename... Fs>
	struct functor_list_t;

	template <typename Base, typename... Fs>
	struct functor_list_t<functor_list_base_t<Base>, Fs...>
		: detail::functor_list2_t<
			functor_list_base_t<std::remove_reference_t<Base>>,
			std::tuple<Fs...>>
	{
		using base_type = detail::functor_list2_t<
			functor_list_base_t<std::remove_reference_t<Base>>,
			std::tuple<Fs...>>;

		using base_type::base_type;
	};

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: detail::functor_list2_t<functor_list_base_t<>, std::tuple<Fs...>>
	{
		using base_type = detail::functor_list2_t<functor_list_base_t<>, std::tuple<Fs...>>;
		using base_type::base_type;
	};

	// deduction guides
	template <typename Base, typename... Fs>
	functor_list_t(functor_list_base_t<Base>, Fs&&...)
		-> functor_list_t<functor_list_base_t<Base>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_base_t<>, std::remove_reference_t<Fs>...>;
}
