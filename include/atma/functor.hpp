#pragma once

#include <type_traits>
#include <tuple>

// functor-call
namespace atma
{
	template <typename... Fwds>
	struct functor_call_fwds_t {};

	struct functor_call_no_fwds_t {};
}

namespace atma::detail
{
	template <typename Fwd, typename Gs, typename F>
	struct functor_call_;

	template <typename F, typename... Fs>
	struct functor_call_<functor_call_no_fwds_t, std::tuple<Fs...>, F>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Args...> && ...)>>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Args...>
		{
			return reinterpret_cast<F&>(const_cast<functor_call_&>(*this))(std::forward<Args>(args)...);
		}
	};

	template <typename... Fwds, typename F, typename... Fs>
	struct functor_call_<functor_call_fwds_t<Fwds...>, std::tuple<Fs...>, F>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Fwds..., Args...> && ...)>>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Fwds..., Args...>
		{
			return reinterpret_cast<F const&>(*this)(reinterpret_cast<Fwds const&>(*this)..., std::forward<Args>(args)...);
		}
	};
}

// functor_list_t
namespace atma::detail
{
	template <typename, typename, typename>
	struct functor_list_;

	// no functors provided/left
	template <typename Fwd, typename Unused>
	struct functor_list_<Fwd, Unused, std::tuple<>>
	{};

	// Fwd: forwards
	// Gs: a list of "previously seen" functors
	// F: functor to create the functor-call operator for
	// Fs: a list of remaining functors to recurse upon
	template <typename Fwd, typename... Gs, typename F, typename... Fs>
	struct functor_list_<Fwd, std::tuple<Gs...>, std::tuple<F, Fs...>>
		: functor_list_<Fwd, std::tuple<Gs..., F>, std::tuple<Fs...>>
		, functor_call_<Fwd, std::tuple<Gs...>, F>
	{};

	// fwds_are_empty_functors is just a compile-time check to catch this as early as possible
	template <typename T> struct fwds_are_empty_functors_t : std::true_type {};

	template <typename... Fwds>
	struct fwds_are_empty_functors_t<functor_call_fwds_t<Fwds...>>
		: std::bool_constant<(std::is_empty_v<Fwds> && ...)>
	{};
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t
		: detail::functor_list_<Fwds, std::tuple<>, std::tuple<Fs...>>
	{
		static_assert(detail::fwds_are_empty_functors_t<Fwds>::value, "all forwarded functors must be empty functors");

		constexpr functor_list_t() = default;

		template <typename... Gs>
		constexpr functor_list_t(Gs&&...)
		{}
	};

	template <typename... Fwds, typename... Fs>
	functor_list_t(functor_call_fwds_t<Fwds...>, Fs&&...) -> functor_list_t<functor_call_fwds_t<Fwds...>, rmref_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...) -> functor_list_t<functor_call_no_fwds_t, rmref_t<Fs>...>;
}
