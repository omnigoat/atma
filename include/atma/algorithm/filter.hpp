#pragma once

#include <atma/bind.hpp>
#include <atma/assert.hpp>
#include <atma/function.hpp>


namespace atma
{
	template <typename R, typename F> struct filtered_range_t;
	template <typename R> struct filtered_range_iterator_t;
}

//======================================================================
// filtered-range
//======================================================================
namespace atma
{
	template <typename R, typename F>
	struct filtered_range_t
	{
		using source_container_t  = std::remove_reference_t<R>;
		using source_iterator_t   = decltype(std::begin(std::declval<source_container_t&>()));
		using storage_container_t = storage_type_t<R>;

		using value_type      = typename source_container_t::value_type;
		using reference       = typename source_container_t::reference;
		using const_reference = typename source_container_t::const_reference;
		using iterator        = filtered_range_iterator_t<transfer_const_t<source_container_t, filtered_range_t>>;
		using const_iterator  = filtered_range_iterator_t<filtered_range_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;

		filtered_range_t(filtered_range_t&&);
		filtered_range_t(filtered_range_t const&) = delete;

		template <typename CC, typename FF>
		filtered_range_t(CC&&, FF&&);

		decltype(auto) source_container()       { return std::forward<storage_container_t>(range_); }
		decltype(auto) source_container() const { return std::forward<storage_container_t>(range_); }
		decltype(auto) predicate() const { return (predicate_); }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

	private:
		// if R is an lvalue, const or otherwise, we too are an lvalue.
		// otherwise, storage-type is a non-reference type, and we are the owner of the container.
		storage_container_t range_;
		F predicate_;
	};
}


//======================================================================
// partial-filtered-range
//======================================================================
namespace atma
{
	template <typename F>
	struct partial_filtered_range_t
	{
		template <typename FF>
		partial_filtered_range_t(FF&& f)
			: fn_(std::forward<FF>(f))
		{}

		template <typename R>
		decltype(auto) operator ()(R&& xs) const
		{
			return filtered_range_t<decltype(xs), F>(std::forward<R>(xs), fn_);
		}

		decltype(auto) predicate() { return (fn_); }

	private:
		F fn_;
	};


	// function composition partial-filter-range against any range
	template <typename F, typename R>
	struct function_composition_override<partial_filtered_range_t<F>, R>
	{
		template <typename RR>
		static decltype(auto) compose(partial_filtered_range_t<F> const& f, RR&& r)
		{
			return filtered_range_t<decltype(r), decltype(f.predicate())>{std::forward<RR>(r), f.predicate()};
		}
	};

	// optimized function composition for partial-filter-range vs filter-range
	template <typename F, typename R, typename G>
	struct function_composition_override<partial_filtered_range_t<F>, filtered_range_t<R, G>>
	{
		template <typename R1, typename R2>
		static decltype(auto) compose(R1&& r1, R2&& r2)
		{
			auto predicate = [f = r1.predicate(), g = r2.predicate()](auto&& x)
				{ return g(std::forward<decltype(x)>(x)) && f(std::forward<decltype(x)>(x)); };

			return filtered_range_t<R, decltype(predicate)>{r2.source_container(), predicate};
		}
	};

	// optimized function composition for two partial filter-ranges
	template <typename F, typename G>
	struct function_composition_override<partial_filtered_range_t<F>, partial_filtered_range_t<G>>
	{
		template <typename FF, typename GG>
		static decltype(auto) compose(FF&& r1, GG&& r2)
		{
			auto predicate = [f = r1.predicate(), g = r2.predicate()](auto&& x)
				{ return g(std::forward<decltype(x)>(x)) && f(std::forward<decltype(x)>(x)); };

			return partial_filtered_range_t<decltype(predicate)>{predicate};
		}
	};
}


//======================================================================
// filtered-range-iterator
//======================================================================
namespace atma
{
	template <typename R>
	struct filtered_range_iterator_t
	{
		using owner_t = std::remove_reference_t<R>;
		using source_iterator_t = typename owner_t::source_iterator_t;

		constexpr static bool is_const = std::is_const_v<owner_t> || std::is_const_v<typename owner_t::value_type>;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = std::conditional_t<is_const, value_type const&, value_type&>;

		filtered_range_iterator_t() = default;
		filtered_range_iterator_t(owner_t*, source_iterator_t const& begin);

		auto operator  *() const -> reference;
		auto operator ++() -> filtered_range_iterator_t&;

	private:
		R* owner_;
		source_iterator_t pos_;

		template <typename C2, typename D2>
		friend auto operator == (filtered_range_iterator_t<C2> const&, filtered_range_iterator_t<D2> const&) -> bool;
	};
}


//======================================================================
// RANGE IMPLEMENTATION
//======================================================================
namespace atma
{
	template <typename R, typename F>
	template <typename CC, typename FF>
	inline filtered_range_t<R, F>::filtered_range_t(CC&& source, FF&& predicate)
		: range_{std::forward<CC>(source)}
		, predicate_(std::forward<FF>(predicate))
	{}

	template <typename R, typename F>
	inline filtered_range_t<R, F>::filtered_range_t(filtered_range_t&& rhs)
		: range_{std::forward<decltype(rhs.range_)>(rhs.range_)}
		, predicate_{rhs.predicate_}
	{}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::begin() -> iterator
	{
		auto i = std::begin(range_);
		while (i != std::end(range_) && !predicate_(*i))
			++i;

		return iterator{this, i};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::end() -> iterator
	{
		return iterator{this, std::end(range_)};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::begin() const -> const_iterator
	{
		auto i = std::begin(range_);
		while (i != std::end(range_) && !predicate_(*i))
			++i;

		return const_iterator{this, i};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::end() const -> const_iterator
	{
		return const_iterator{this, std::end(range_)};
	}
}


//======================================================================
// ITERATOR IMPLEMENTATION
//======================================================================
namespace atma
{
	template <typename R>
	inline filtered_range_iterator_t<R>::filtered_range_iterator_t(owner_t* owner, source_iterator_t const& begin)
		: owner_(owner), pos_(begin)
	{
	}

	template <typename R>
	inline auto filtered_range_iterator_t<R>::operator *() const -> reference
	{
		return *pos_;
	}

	template <typename R>
	inline auto filtered_range_iterator_t<R>::operator ++() -> filtered_range_iterator_t&
	{
		ATMA_ASSERT(pos_ != owner_->source_container().end());

		do {
			++pos_;
		} while (pos_ != owner_->source_container().end() && !std::invoke(owner_->predicate(), *pos_));

		return *this;
	}

	template <typename R, typename D>
	inline auto operator == (filtered_range_iterator_t<R> const& lhs, filtered_range_iterator_t<D> const& rhs) -> bool
	{
		static_assert(std::is_same_v<std::remove_const_t<R>, std::remove_const_t<D>>);
		ATMA_ASSERT(lhs.owner_ == rhs.owner_);
		return lhs.pos_ == rhs.pos_;
	}

	template <typename R, typename D>
	inline auto operator != (filtered_range_iterator_t<R> const& lhs, filtered_range_iterator_t<D> const& rhs) -> bool
	{
		static_assert(std::is_same_v<std::remove_const_t<R>, std::remove_const_t<D>>);
		return !operator == (lhs, rhs);
	}
}


//======================================================================
// NONMEMBER FUNCTIONS
//======================================================================
namespace atma
{
	// filter: f vs r
	template <typename F, typename R>
	inline auto filter(F&& predicate, R&& container)
		{ return filtered_range_t<decltype(container), F>{std::forward<R>(container), std::forward<F>(predicate)}; }

	// filter: f vs filtered-range<r, g>
	template <typename F, typename R, typename F2>
	inline auto filter(F&& predicate, filtered_range_t<R, F2> const& container)
		{ return partial_filtered_range_t<F>{std::forward<F>(predicate)} % container; }

	template <typename F, typename R, typename F2>
	inline auto filter(F&& predicate, filtered_range_t<R, F2>& container)
		{ return partial_filtered_range_t<F>{std::forward<F>(predicate)} % container; }

	template <typename F, typename R, typename F2>
	inline auto filter(F&& predicate, filtered_range_t<R, F2>&& container)
		{ return partial_filtered_range_t<F>{std::forward<F>(predicate)} % std::move(container); }

	// filter: member vs r
	template <typename M, typename C, typename R>
	inline auto filter(M C::*m, R&& range)
	{
		auto f = atma::bind([](M C::*m, auto&& x) -> bool { return x.*m; }, m, arg1);
		return filtered_range_t<decltype(range), decltype(f)>{std::forward<R>(range), f};
	}

	// partial-filter: f
	template <typename F>
	inline auto filter(F&& predicate)
	{
		return partial_filtered_range_t<F>{predicate};
	}
}
