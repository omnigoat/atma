#pragma once

#include <type_traits>

#define LAMBDA_REQUIRES(...) \
	-> std::enable_if_t<::atma::concepts::detail::all_true_v<__VA_ARGS__>, void>

#define RETURN_TYPE_IF(type, ...) \
	std::enable_if_t<::atma::concepts::detail::all_true_v<__VA_ARGS__>, type>

#define RETURN_TYPEN_IF(type, ...) \
	std::enable_if_t<::atma::concepts::detail::all_true_v<__VA_ARGS__>, BOOST_PP_EXPAND##type>



// functor-call
namespace atma
{
	template <typename... Fwds>
	struct functor_call_fwds_t {};

	struct functor_call_no_fwds_t {};
}

namespace atma::detail
{
	template <typename Fwd, typename F, typename... Fs>
	struct functor_call_;

	template <typename F, typename... Fs>
	struct functor_call_<functor_call_no_fwds_t, F, Fs...>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Args...> && ...)>>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Args...>
		{
			// don't go through std::invoke because all empty functors should be callable
			// through operator(), and it helps with debugging
			return reinterpret_cast<F&>(const_cast<functor_call_&>(*this))(std::forward<Args>(args)...);
		}
	};

	template <typename... Fwds, typename F, typename... Fs>
	struct functor_call_<functor_call_fwds_t<Fwds...>, F, Fs...>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Args...> && ...)>>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Fwds..., Args...>
		{
			// don't go through std::invoke because all empty functors should be callable
			// through operator(), and it helps with debugging
			return reinterpret_cast<F const&>(*this)(
				reinterpret_cast<Fwds const&>(*this)...,
				std::forward<Args>(args)...);
		}
	};
}

// multi-functor
namespace atma::detail
{
	template <typename, typename, typename>
	struct multi_functor_;

	template <typename Fwd, typename... Gs>
	struct multi_functor_<Fwd, meta::list<Gs...>, meta::list<>>
	{};

	template <typename Fwd, typename... Gs, typename F, typename... Fs>
	struct multi_functor_<Fwd, meta::list<Gs...>, meta::list<F, Fs...>>
		: multi_functor_<Fwd, meta::list<Gs..., F>, meta::list<Fs...>>
		, functor_call_<Fwd, F, Gs...>
	{};
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct multi_functor_t
		: detail::multi_functor_<Fwds, meta::list<>, meta::list<rmref_t<Fs>...>>
	{
		constexpr multi_functor_t() = default;

		// we don't actually care about the arguments
		template <typename... Gs>
		constexpr multi_functor_t(Gs&&...)
		{}
	};

	template <typename... Fwds, typename... Fs>
	multi_functor_t(functor_call_fwds_t<Fwds...>, Fs&&...) -> multi_functor_t<functor_call_fwds_t<Fwds...>, Fs...>;

	template <typename... Fs>
	multi_functor_t(Fs&&...) -> multi_functor_t<functor_call_no_fwds_t, Fs...>;
}


