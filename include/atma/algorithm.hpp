#pragma once

#include <atma/tuple.hpp>

#include <algorithm>
#include <optional>
#include <ranges>

import atma.types;
import atma.vector;
import atma.meta;

namespace atma::functors
{
	template <typename Orig, auto member>
	struct member_
	{
		constexpr member_() = default;

		template <typename... Args>
		auto operator ()(Args&&... args) const
		requires meta::all_same<rm_ref_t<Args>...>::value
		{
			auto f = [](auto&& x) { return (x.*member); };

			return Orig::apply(f(std::forward<Args>(args))...);
		}

		template <typename... Args>
		auto operator ()(Args&&... args) const
		requires !meta::all_same<rm_ref_t<Args>...>::value
		{
			auto f = [](auto&& x) { return (x.*member); };

			return Orig::apply(f(std::forward<Args>(args))...);
		}
	};

	inline constexpr struct add_fn
	{
		template <typename A, typename B>
		requires requires(A a, B b) { {a + b}; }
		auto operator ()(A&& a, B&& b) const
		{
			return a + b;
		}

		template <typename A, typename B>
		static auto apply(A&& a, B&& b)
		requires requires{ {operator +(a, b)}; }
		{
			return operator +(std::forward<A>(a), std::forward<B>(b));
		}

		template <auto member>
		static member_<add_fn, member> member;
		
	} add;
}

namespace atma::functors
{
	//template <typename 

	//functors::add.member<&node_info_t<RT>::characters>
}


namespace atma
{
	template <typename C>
	struct range_t
	{
		using self_t             = range_t<C>;
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t  = decltype(std::declval<source_container_t>().begin());

		using value_type      = typename source_container_t::value_type;
		using reference       = typename source_container_t::reference;
		using const_reference = typename source_container_t::const_reference;
		using iterator        = typename source_container_t::iterator;
		using const_iterator  = typename source_container_t::const_iterator;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;

		// if our source container is const, then even if we are not const, we can only provide const iterators.
		using either_iterator = std::conditional_t<std::is_const<source_container_t>::value, const_iterator, iterator>;

		template <typename CC>
		range_t(CC&&, source_iterator_t const&, source_iterator_t const&);
		range_t(range_t const&) = delete;

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> either_iterator;
		auto end() -> either_iterator;

	private:
		C container_;
		source_iterator_t begin_, end_;
	};

	template <typename XS>
	auto slice(XS&& xs, uint begin, uint stop) -> range_t<XS>
	{
		return {xs, xs.begin() + begin, xs.begin() + stop};
	}



	template <typename C>
	template <typename CC>
	range_t<C>::range_t(CC&& source, source_iterator_t const& begin, source_iterator_t const& end)
		: container_(std::forward<CC>(source)), begin_(begin), end_(end)
	{
	}

	template <typename C>
	auto range_t<C>::begin() -> either_iterator
	{
		return begin_;
	}

	template <typename C>
	auto range_t<C>::end() -> either_iterator
	{
		return end_;
	}

	template <typename C>
	auto range_t<C>::begin() const -> const_iterator
	{
		return begin_;
	}

	template <typename C>
	auto range_t<C>::end() const -> const_iterator
	{
		return end_;
	}


	//
	// as_vector
	// -----------
	//   turns a range into an atma::vector
	//
	constexpr inline struct as_vector_t
	{
		template <std::ranges::range R>
		auto operator () (R&& range) const
		{
			return atma::vector{std::ranges::begin(range), std::ranges::end(range)};
		}
	} as_vector;

	template <std::ranges::range R>
	inline auto operator | (R&& range, as_vector_t const&)
	{
		return atma::vector<std::ranges::range_value_t<R>>{range.begin(), range.end()};
	}


	//
	// find_in
	// -----------
	//
	template <std::ranges::range R>
	inline auto find_in(R&& range, std::ranges::range_value_t<R> const& x)
	{
		return std::find(std::begin(range), std::end(range), x);
	}


	//
	// foldl
	// -----------
	//
	template <typename R, typename F>
	inline auto foldl(R const& range, F fn) -> typename std::decay_t<R>::value_type
	{
		ATMA_ASSERT(!range.empty());

		auto r = range.first();

		for (auto i = ++range.begin(); i != range.end(); ++i)
			r = fn(r, *i);
		
		return r;
	}

	template <typename R, typename I, typename F>
	inline auto foldl(R const& range, I&& initial, F fn) -> std::decay_t<I>
	{
		auto r = std::forward<I>(initial);

		for (auto i = range.begin(); i != range.end(); ++i)
			r = fn(r, *i);

		return r;
	}

	template <typename IT, typename I, typename F>
	inline auto foldl(IT begin, IT end, I&& initial, F fn) -> std::decay_t<I>
	{
		auto r = std::forward<I>(initial);

		for (auto i = begin; i != end; ++i)
			r = fn(r, *i);

		return r;
	}



	//
	// singular_result
	// -----------------
	//   returns a value if all elements in the range map to the same value,
	//   otherwise returns empty
	//
	template <std::ranges::range Range, typename F>
	requires std::is_invocable_v<F, std::ranges::range_value_t<Range>>
	inline auto singular_result(Range const& range, F fn) -> std::optional<std::invoke_result_t<F, std::ranges::range_value_t<Range>>>
	{
		if (std::empty(range))
		{
			return {};
		}
		else
		{
			auto iter = std::begin(range);
			decltype(auto) r = fn(*iter);

			for (auto end = std::end(range); iter != end; ++iter)
			{
				decltype(auto) r2 = fn(*iter);
				if (r != r2)
				{
					return {};
				}
			}

			return {r};
		}
	}
}
