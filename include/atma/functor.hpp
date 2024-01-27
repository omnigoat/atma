#if 1
#pragma once

#include <type_traits>
#include <tuple>

namespace atma
{
	template <typename... Fwds>
	struct functor_list_fwds_t {};
}

// Gs: "previously seen" functors, F: "functor in question", Fs: "yet to evaluate"
namespace atma::detail
{
	template <typename Fwds, typename Gs, typename Fs>
	struct functor_call_impl;

	template <typename FwdsUnused, typename GsUnused>
	struct functor_call_impl<FwdsUnused, GsUnused, std::tuple<>>
	{ }; // termination condition

	template <typename... Fwds, typename... Gs, typename F, typename... Fs>
	struct functor_call_impl<functor_list_fwds_t<Fwds...>, std::tuple<Gs...>, std::tuple<F, Fs...>>
		: functor_call_impl<functor_list_fwds_t<Fwds...>, std::tuple<Gs..., F>, std::tuple<Fs...>>
	{
		//constexpr auto operator ()(auto&&... args) const
		//requires (!std::invocable<Gs, Fwds..., decltype(args)...> && ...) && std::invocable<F, Fwds..., decltype(args)...>
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...)
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Fwds..., Args...>
		{
			return reinterpret_cast<F const&>(*this)(reinterpret_cast<Fwds const&>(*this)..., std::forward<Args>(args)...);
		}
	};

	template <typename Fwds, typename Gs, typename F>
	struct functor_call_impl2;

	template <typename... Fwds, typename... Gs, typename F>
	struct functor_call_impl2<functor_list_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...)
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Fwds..., Args...>
		{
			return reinterpret_cast<F const&>(*this)(reinterpret_cast<Fwds const&>(*this)..., std::forward<Args>(args)...);
		}
	};

	template <typename... Ts>
	struct select_last
	{
		using body_type = std::tuple<std::tuple_element_t<sizeof...(Ts)-1, std::tuple<Ts...>>>;
		using last_type = typename decltype((std::type_identity_t<Ts>{}, ...))::type;
	};

	template <typename Tuple, typename IdxTs>
	struct tuple_select;

	template <typename Tuple, size_t... Idxs>
	struct tuple_select<Tuple, std::index_sequence<Idxs...>>
	{
		using type = std::tuple<std::tuple_element_t<Idxs, Tuple>...>;
		//using gs_type = std::tuple<std::tuple_element_t<sizeof...(Idxs)-1, tuple>>;
	};

	template <typename Tuple, typename IdxTs>
	using tuple_select_t = typename tuple_select<Tuple, IdxTs>::type;



	template <typename Fwds, typename Fs>
	struct functor_call_impl3;

	template <typename Fwds, typename F, typename Fs>
	struct functor_call_impl32;

	template <typename... Fwds, typename F, typename... Gs>
	struct functor_call_impl32<functor_list_fwds_t<Fwds...>, F, std::tuple<Gs...>>
	{
		template <typename... Args>
		requires (!std::invocable<Gs, Fwds..., Args...> && ...) // && std::invocable<F, Fwds..., Args...>
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Fwds..., Args...>
		{
			return reinterpret_cast<F const&>(*this)(reinterpret_cast<Fwds const&>(*this)..., std::forward<Args>(args)...);
		}
	};

	template <typename... Fwds, typename... Fs>
	struct functor_call_impl3<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>
		: functor_call_impl32<functor_list_fwds_t<Fwds...>,
			std::tuple_element_t<sizeof...(Fs)-1, std::tuple<Fs...>>,
			tuple_select_t<std::tuple<Fs...>, std::make_index_sequence<sizeof...(Fs)-1>>>
	{};

#if 0
	template <typename, typename, typename>
	struct functor_list_;

	template <typename FwdsUnused, typename GsUnused>
	struct functor_list_<FwdsUnused, GsUnused, std::tuple<>>
	{}; // termination

	template <typename Fwds, typename... Gs, typename F, typename... Fs>
	struct functor_list_<Fwds, std::tuple<Gs...>, std::tuple<F, Fs...>>
		: functor_list_<Fwds, std::tuple<Gs..., F>, std::tuple<Fs...>>
		, functor_call_impl2<Fwds, std::tuple<Gs...>, F>
	{};

	// : detail::functor_calls<Fwds, std::tuple<>, std::tuple<F0, F1, F2, F3>>...
	// : detail::functor_calls<Fwds, std::tuple<F0>, std::tuple<F1, F2, F3>>...
#elif 0
	template <typename, typename, typename>
	struct functor_list_;

	template <typename Fwds, typename... Gs, typename F, typename... Fs>
	struct functor_list_<Fwds, std::tuple<Gs...>, std::tuple<F, Fs...>>
		: functor_call_impl<Fwds, std::tuple<Gs...>, std::tuple<F, Fs...>>
	{};
#else
	

	template <typename, typename> struct splat;

	template <typename Fs, typename... IdxTs>
	struct splat<Fs, std::tuple<IdxTs...>>
	{
		using type = std::tuple<tuple_select_t<Fs, IdxTs>...>;
	};

	template <typename, typename, typename>
	struct functor_list_3;

	template <typename Fwds, typename... Fs, size_t... Idxs>
	struct functor_list_3<Fwds, std::tuple<Fs...>, std::index_sequence<Idxs...>>
	{
		using idxs_type = std::tuple<std::make_index_sequence<Idxs>...>;
		using type = typename splat<std::tuple<Fs...>, idxs_type>::type;
	};


	template <typename Fwds, typename FGs>
	struct functor_list_4;

	template <typename Fwds, typename... FGs>
	struct functor_list_4<Fwds, std::tuple<FGs...>>
		: functor_call_impl3<Fwds, FGs>...
	{};

	template <typename Fwds, typename Fs, typename Idxs>
	struct functor_list_2;

	template <typename Fwds, typename Fs, size_t... Idxs>
	struct functor_list_2<Fwds, Fs, std::index_sequence<Idxs...>>
		: functor_list_4<Fwds, typename functor_list_3<Fwds, Fs, std::index_sequence<(Idxs+1)...>>::type>
	{};


	template <typename, typename>
	struct functor_list_;

	template <typename Fwds, typename... Fs>
	struct functor_list_<Fwds, std::tuple<Fs...>>
		: functor_list_2<Fwds, std::tuple<Fs...>, std::make_index_sequence<sizeof...(Fs)>>
	{};
#endif
}

namespace atma
{
	template <typename Fwds, typename... Fs>
	struct functor_list_t;

	template <typename... Fs>
	struct functor_list_t<Fs...>
		: functor_list_t<functor_list_fwds_t<>, Fs...>
	{ };

	template <typename... Fwds, typename... Fs>
	struct functor_list_t<functor_list_fwds_t<Fwds...>, Fs...>
		: detail::functor_list_<functor_list_fwds_t<Fwds...>, std::tuple<Fs...>>
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

#else
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

	template <typename F, typename... Gs>
	struct functor_call_<functor_call_no_fwds_t, std::tuple<Gs...>, F>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Args...> && ...)
		constexpr auto operator ()(Args&&... args) const -> std::invoke_result_t<F, Args...>
		{
			return reinterpret_cast<F&>(const_cast<functor_call_&>(*this))(std::forward<Args>(args)...);
		}
	};

	template <typename... Fwds, typename F, typename... Gs>
	struct functor_call_<functor_call_fwds_t<Fwds...>, std::tuple<Gs...>, F>
	{
		static_assert(std::is_empty_v<F>, "functor must be empty");

		template <typename... Args>
		requires (!std::is_invocable_v<Gs, Fwds..., Args...> && ...)
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
	functor_list_t(functor_call_fwds_t<Fwds...>, Fs&&...) -> functor_list_t<functor_call_fwds_t<Fwds...>, std::remove_reference_t<Fs>...>;

	template <typename... Fs>
	functor_list_t(Fs&&...) -> functor_list_t<functor_call_no_fwds_t, std::remove_reference_t<Fs>...>;
}

#endif