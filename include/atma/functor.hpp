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

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Fwds..., Args...> && ...)>>
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

// functor_list_t
namespace atma::detail
{
	template <typename, typename, typename>
	struct functor_list_;

	template <typename Fwd, typename... Gs>
	struct functor_list_<Fwd, meta::list<Gs...>, meta::list<>>
	{};

	template <typename Fwd, typename... Gs, typename F, typename... Fs>
	struct functor_list_<Fwd, meta::list<Gs...>, meta::list<F, Fs...>>
		: functor_list_<Fwd, meta::list<Gs..., F>, meta::list<Fs...>>
		, functor_call_<Fwd, F, Gs...>
	{};
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t
		: detail::functor_list_<Fwds, meta::list<>, meta::list<rmref_t<Fs>...>>
	{
		constexpr functor_list_t() = default;

		// we don't actually care about the arguments
		template <typename... Gs>
		constexpr functor_list_t(Gs&&...)
		{}
	};

	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_call_fwds_t<Fwds...>, Fs&&...) -> functor_list_t<functor_call_fwds_t<Fwds...>, Fs...>;

	template <typename... Fs>
	functor_list_t(Fs&&...) -> functor_list_t<functor_call_no_fwds_t, Fs...>;
}


