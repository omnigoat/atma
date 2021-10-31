#pragma once

#include <utility>

import atma.types;

// composited_abstract_t
namespace atma
{
	template <typename F, typename G>
	struct composited_abstract_t
	{
		template <typename FF, typename GG>
		composited_abstract_t(FF&& f, GG&& g)
			: f_{std::forward<FF>(f)}
			, g_{std::forward<GG>(g)}
		{}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) const
		{
			return f_(g_(std::forward<Args>(args)...));
		}

	private:
		F f_;
		G g_;
	};

	// deduction guide!
	template <typename F, typename G>
	composited_abstract_t(F&& f, G&& g) ->
		composited_abstract_t<storage_type_t<F>, storage_type_t<G>>;
}


// composited_concrete_t
namespace atma
{
	template <typename F, typename G, typename... Args>
	struct composited_concrete_t
	{
		template <typename FF, typename GG>
		composited_concrete_t(FF&& f, GG&& g)
			: f_{std::forward<FF>(f)}
			, g_{std::forward<GG>(g)}
		{}

		decltype(auto) operator()(Args... args) const
		{
			return f_(g_(args...));
		}

	private:
		F f_;
		G g_;
	};

	// deduction guide!
	template <typename... Args, typename F, typename G>
	composited_concrete_t(F&& f, G&& g) ->
		composited_concrete_t<storage_type_t<F>, storage_type_t<G>, Args...>;
}


//
//  compose_impl
//  --------------
//    creates a functor that calls one function, then passes that result to another.
//    if possible, it is constructed with a concrete functor-operator. this
//    occurs when G has a concrete functor-operator. otherwise, a templated
//    functor-operator is used.
//
namespace atma::detail
{
	template <typename>
	struct composer_ii_x;

	template <typename... Args>
	struct composer_ii_x<std::tuple<Args...>>
	{
		template <typename F, typename G>
		static decltype(auto) go(F&& f, G&& g)
		{
			return composited_concrete_t<Args...>{std::forward<F>(f), std::forward<G>(g)};
		}
	};

	struct composer_x
	{
		template <typename F, typename G>
		static decltype(auto) go(F&& f, G&& g)
		{
			return composer_ii_x<typename function_traits<std::decay_t<G>>::tupled_args_type>::go(std::forward<F>(f), std::forward<G>(g));
		}
	};

	template <typename F, typename G>
	inline decltype(auto) compose_impl_concrete(F&& f, G&& g)
	{
		return composer_x::go(std::forward<F>(f), std::forward<G>(g));
	}

	template <typename F, typename G>
	inline auto compose_impl_abstract(F&& f, G&& g)
	{
		return composited_abstract_t{std::forward<F>(f), std::forward<G>(g)};
	}
}

namespace atma
{
	template <typename F>
	struct functionally_composable;



	//
	//  function_composition_override
	//  -------------------------------
	//    allows users to override how function-composition between two invokables is implemented
	//
	//    defining this is all that's required to enable function-composition between the two
	//    types, even if neither type has been explicitly allowed for general-use composition
	//
	template <typename F, typename G, typename = std::void_t<>>
	struct function_composition_implementation_t {};

	template <typename F, typename G>
	struct function_composition_implementation_t<F, G, std::void_t<
		decltype(functionally_composable<F>()),
		decltype(functionally_composable<G>())>>
	{
		template <typename FF, typename GG>
		static auto compose(FF&& f, GG&& g)
		{
			if constexpr (is_callable_v<std::remove_reference_t<GG>>)
			{
				return detail::compose_impl_concrete(std::forward<F>(f), std::forward<G>(g));
			}
			else
			{
				return detail::compose_impl_abstract(std::forward<F>(f), std::forward<G>(g));
			}
		}
	};


	//
	//  function_composition_override
	//  -------------------------------
	//    allows users to override how function-composition between two invokables is implemented
	//
	//    defining this is all that's required to enable function-composition between the two
	//    types, even if neither type has been explicitly allowed for general-use composition
	//
	template <typename F, typename G>
	struct function_composition_override : function_composition_implementation_t<F, G>
	{};

	template <typename F, typename G>
	struct function_composition_t : function_composition_override<F, G>
	{};


	//
	// compose
	// ---------
	//   takes two callable things and composes them
	//
	template <typename F, typename G>
	inline auto compose(F&& f, G&& g) {
		return function_composition_t<std::remove_reference_t<F>, std::remove_reference_t<G>>
			::compose(std::forward<F>(f), std::forward<G>(g));
	}



}
