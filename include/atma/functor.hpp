#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename Datum = void>
	struct functor_list_datum_t
	{
		constexpr functor_list_datum_t() = default;

		template <typename DatumFwd>
		constexpr functor_list_datum_t(DatumFwd&& rhs)
			: fwd{std::forward<DatumFwd>(rhs)}
		{}

		Datum fwd;
	};

	template <>
	struct functor_list_datum_t<>
	{
		constexpr functor_list_datum_t() = default;
	};

	template <typename T>
	functor_list_datum_t(T&) -> functor_list_datum_t<T&>;

	template <typename T>
	functor_list_datum_t(T const&&) -> functor_list_datum_t<T>;
}

// Fwds: the forwards, Gs: "previously seen" functors, F: "functor in question", Rs: "yet to evaluate"
namespace atma::detail
{
	template <typename Datum, typename Gs, typename F>
	struct functor_delegate_;

	template <typename... Gs, typename F>
	struct functor_delegate_<functor_list_datum_t<>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Args...> && ...) && std::is_invocable_v<F, Args...>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return F{}(std::forward<Args>(args)...);
		}
	};

	template <typename Datum, typename... Gs, typename F>
	struct functor_delegate_<functor_list_datum_t<Datum>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Datum&, Args...> && ...) && std::is_invocable_v<F, Datum&, Args...>
		constexpr decltype(auto) operator ()(Datum& datum, Args&&... args) const
		{
			return F{}(datum, std::forward<Args>(args)...);
		}
	};
}

namespace atma::detail
{
	template <typename Datum, typename Gs, typename Rs>
	struct functor_list_;

	template <typename DatumWrap, typename Gs>
	struct functor_list_<DatumWrap, Gs, std::tuple<>>
	{};

	template <typename DatumWrap, typename... Gs, typename F, typename... Rs>
	struct functor_list_<DatumWrap, std::tuple<Gs...>, std::tuple<F, Rs...>>
		: functor_list_<DatumWrap, std::tuple<Gs..., F>, std::tuple<Rs...>>
		, functor_delegate_<DatumWrap, std::tuple<Gs...>, F>
	{};
}

namespace atma::detail
{
	template <typename Datum, typename Fs>
	struct functor_list2_t;

	template <typename Fs>
	struct functor_list2_t<functor_list_datum_t<>, Fs>
		: detail::functor_list_<functor_list_datum_t<>, std::tuple<>, Fs>
	{
		constexpr functor_list2_t(auto&&...) noexcept
		{}
	};

	template <typename Datum, typename Fs>
	struct functor_list2_t<functor_list_datum_t<Datum>, Fs>
		: detail::functor_list_<functor_list_datum_t<Datum>, std::tuple<>, Fs>
	{
		using base_type = detail::functor_list_<functor_list_datum_t<Datum>, std::tuple<>, Fs>;

		template <typename DatumFwd>
		constexpr functor_list2_t(DatumFwd&& datum, auto&&...) noexcept
			: datum_{std::forward<DatumFwd>(datum).fwd}
		{}

		template <typename... Args>
		constexpr decltype(auto) operator ()(Args&&... args) const
		{
			return std::invoke(static_cast<base_type const&>(*this), datum_, std::forward<Args>(args)...);
		}

		auto datum() const -> Datum const& { return this->datum_; }

	private:
		Datum datum_;
	};
}

namespace atma
{
	template <typename Datum, typename... Fs>
	struct functor_list_t;

	template <typename Datum, typename... Fs>
	struct functor_list_t<functor_list_datum_t<Datum>, Fs...>
		: detail::functor_list2_t<functor_list_datum_t<Datum>, std::tuple<Fs...>>
	{
		using base_type = detail::functor_list2_t<functor_list_datum_t<Datum>, std::tuple<Fs...>>;
		using base_type::base_type;
	};

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: functor_list_t<functor_list_datum_t<>, Fs...>
	{};

	template <typename Datum, typename... Fs>
	functor_list_t(functor_list_datum_t<Datum>, Fs&&...)
		-> functor_list_t<functor_list_datum_t<Datum>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...)
		-> functor_list_t<functor_list_datum_t<>, std::remove_reference_t<Fs>...>;
}
