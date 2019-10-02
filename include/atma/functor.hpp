#pragma once

#include <type_traits>


#define RETURN_TYPE_IF(type, ...) \
	std::enable_if_t<__VA_ARGS__, type>


namespace atma
{
	// functor-call
	namespace detail
	{
		template <typename F, typename... Fs>
		struct functor_call_
		{
			static_assert(std::is_empty_v<F>, "functor must be empty");

			template <typename... Args, typename = std::enable_if_t<(!std::is_invocable_v<Fs, Args...> && ...)>>
			constexpr auto operator ()(Args && ... args) const -> std::invoke_result_t<F, Args...>
			{
				return std::invoke(reinterpret_cast<F const&>(const_cast<functor_call_&>(*this)), std::forward<Args>(args)...);
			}
		};
	}

	// multi-functor
	namespace detail
	{
		template <typename, typename, typename>
		struct multi_functor_;

		template <typename... Fs, typename F>
		struct multi_functor_<atma::meta::list<Fs...>, F, atma::meta::list<>>
			: functor_call_<F, Fs...>
		{};

		template <typename... Fs, typename F, typename G, typename... Gs>
		struct multi_functor_<atma::meta::list<Fs...>, F, atma::meta::list<G, Gs...>>
			: multi_functor_<meta::list<Fs..., F>, G, meta::list<Gs...>>
			, functor_call_<F, Fs...>
		{};
	}

	template <typename F, typename... Fs>
	struct multi_functor_t
		: detail::multi_functor_<meta::list<>, F, meta::list<Fs...>>
	{};
}


// make_functor
namespace atma
{
	template <typename... Fs>
	constexpr auto make_functor(Fs&& ... fs) -> multi_functor_t<std::remove_reference_t<Fs>...>
	{
		return {};
	}
}

