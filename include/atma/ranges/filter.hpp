#pragma once

#include <atma/ranges/core.hpp>
#include <atma/assert.hpp>

import atma.types;

// forward-declares
namespace atma
{
	template <typename, typename> struct filtered_range_t;
	template <typename, typename> struct filtered_range_iterator_t;
	template <typename> struct filter_functor_t;
}

// specialization for std::common_type
namespace std
{
	template <typename O, typename I, typename J>
	struct common_type<atma::filtered_range_iterator_t<O, I>, atma::filtered_range_iterator_t<O, J>>
	{
		using type = atma::filtered_range_iterator_t<O, common_type_t<I, J>>;
	};
}

// is_filtered_range
namespace atma::detail
{
	template <typename T>
	inline constexpr bool is_filtered_range_v = atma::is_instance_of_v<T, filtered_range_t>;
}


// is_filter_functor
namespace atma::detail
{
	template <typename T>
	inline constexpr bool is_filter_functor_v = atma::is_instance_of_v<T, filter_functor_t>;
}



// filtered-range
namespace atma
{
	template <typename R, typename F>
	struct filtered_range_t
	{
		using target_range_t     = std::remove_reference_t<R>;
		using storage_range_t    = storage_type_t<R>;
		using storage_functor_t  = storage_type_t<F>;

		// iterator types
		using begin_iterator_type       = filtered_range_iterator_t<filtered_range_t, decltype(std::begin(std::declval<target_range_t&>()))>;
		using end_iterator_type         = filtered_range_iterator_t<filtered_range_t, decltype(std::end(std::declval<target_range_t&>()))>;
		using begin_const_iterator_type = filtered_range_iterator_t<filtered_range_t, decltype(std::cbegin(std::declval<target_range_t&>()))>;
		using end_const_iterator_type   = filtered_range_iterator_t<filtered_range_t, decltype(std::cend(std::declval<target_range_t&>()))>;

		using target_iterator_t       = std::common_type_t<begin_iterator_type, end_iterator_type>;
		using target_const_iterator_t = std::common_type_t<begin_const_iterator_type, end_const_iterator_type>;

		// concept: Container
		using value_type      = typename target_range_t::value_type;
		using reference       = typename target_range_t::reference;
		using const_reference = typename target_range_t::const_reference;
		using iterator        = filtered_range_iterator_t<filtered_range_t, target_iterator_t>;
		using const_iterator  = filtered_range_iterator_t<filtered_range_t, target_const_iterator_t>;
		using difference_type = typename target_range_t::difference_type;
		using size_type       = typename target_range_t::size_type;

		filtered_range_t(filtered_range_t&&);
		filtered_range_t(filtered_range_t const&) = delete;

		template <typename RR, typename FF>
		filtered_range_t(RR&&, FF&&);

		decltype(auto) target_range() &  { return (range_); }
		decltype(auto) target_range() && { return std::forward<storage_range_t>(range_); }
		decltype(auto) target_range() const& { return (range_); }
		decltype(auto) predicate() &  { return (predicate_); }
		decltype(auto) predicate() && { return std::forward<storage_functor_t>(predicate_); }
		decltype(auto) predicate() const& { return (predicate_); }

		auto begin() -> begin_iterator_type;
		auto begin() const -> begin_const_iterator_type;
		auto end() -> end_iterator_type;
		auto end() const -> end_const_iterator_type;

	private:
		storage_range_t range_;
		storage_functor_t predicate_;
	};

	template <typename R, typename F>
	filtered_range_t(R&& r, F&& f) -> filtered_range_t<R, F>;
}


// filter-functor
namespace atma
{
	template <typename F>
	struct filter_functor_t
	{
		using storage_functor_t = storage_type_t<F>;

		template <typename FF>
		filter_functor_t(FF&& f)
			: fn_(std::forward<FF>(f))
		{}

		template <typename R>
		decltype(auto) operator ()(R&& xs) {
			return filtered_range_t{std::forward<R>(xs), predicate()};
		}

		template <typename R>
		decltype(auto) operator ()(R&& xs) const {
			return filtered_range_t{std::forward<R>(xs), predicate()};
		}

		decltype(auto) predicate() &  { return (fn_); }
		decltype(auto) predicate() && { return std::forward<storage_functor_t>(fn_); }

	private:
		storage_functor_t fn_;
	};

	template <typename F>
	filter_functor_t(F&& f) -> filter_functor_t<F>;
}


// filtered-range-iterator
namespace atma
{
	template <typename Owner, typename TargetIter>
	struct filtered_range_iterator_t
	{
		using owner_t = Owner;
		using target_iterator_t = TargetIter;

		constexpr static bool is_const = std::is_const_v<typename std::iterator_traits<target_iterator_t>::value_type>; // || std::is_const_v<typename owner_t::value_type>;

		// concept: Iterator
		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename std::iterator_traits<target_iterator_t>::value_type;
		using difference_type   = ptrdiff_t;
		using pointer           = typename std::iterator_traits<target_iterator_t>::pointer;
		using reference         = typename std::iterator_traits<target_iterator_t>::reference; // std::conditional_t<is_const, typename owner_t::const_reference, typename owner_t::reference>;

		filtered_range_iterator_t() = default;
		filtered_range_iterator_t(filtered_range_iterator_t const&) = default;
		filtered_range_iterator_t(filtered_range_iterator_t&&) = default;

		auto operator = (filtered_range_iterator_t const& rhs) -> filtered_range_iterator_t&
		{
			owner_ = rhs.owner_;
			pos_ = rhs.pos_;
			return *this;
		}

		template <typename Iter> filtered_range_iterator_t(owner_t*, Iter&&);
		template <typename Iter> filtered_range_iterator_t(filtered_range_iterator_t<Owner, Iter> const&);

		auto operator  *() const -> reference const;
		auto operator  *() -> reference;
		auto operator ++() -> filtered_range_iterator_t&;
		auto operator ++(int) -> filtered_range_iterator_t;

		auto owner() const -> owner_t const* { return owner_; }
		auto target() const -> target_iterator_t const& { return pos_; }

	private:
		owner_t* owner_ = nullptr;
		target_iterator_t pos_;
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
	inline auto filtered_range_t<R, F>::begin() -> begin_iterator_type
	{
		auto i = std::begin(range_);
		while (i != std::end(range_) && !range_function_invoke(predicate_, *i))
			++i;

		return begin_iterator_type{this, i};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::end() -> end_iterator_type
	{
		return end_iterator_type{this, std::end(range_)};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::begin() const -> begin_const_iterator_type
	{
		auto i = std::begin(range_);
		while (i != std::end(range_) && !range_function_invoke(predicate_, *i))
			++i;

		return begin_const_iterator_type{this, i};
	}

	template <typename R, typename F>
	inline auto filtered_range_t<R, F>::end() const -> end_const_iterator_type
	{
		return end_const_iterator_type{this, std::end(range_)};
	}
}


//======================================================================
// ITERATOR IMPLEMENTATION
//======================================================================
namespace atma
{
	template <typename O, typename I>
	template <typename Iter>
	inline filtered_range_iterator_t<O, I>::filtered_range_iterator_t(owner_t* owner, Iter&& begin)
		: owner_{owner}
		, pos_{std::forward<Iter>(begin)}
	{}

	template <typename O, typename I>
	inline auto filtered_range_iterator_t<O, I>::operator *() -> reference
	{
		return *pos_;
	}

	template <typename O, typename I>
	inline auto filtered_range_iterator_t<O, I>::operator *() const -> reference const
	{
		return *pos_;
	}

	template <typename O, typename I>
	inline auto filtered_range_iterator_t<O, I>::operator ++() -> filtered_range_iterator_t&
	{
		auto target_range_end = owner_->target_range().end();

		ATMA_ASSERT(pos_ != target_range_end);

		do {
			++pos_;
		} while (pos_ != target_range_end && !range_function_invoke(owner_->predicate(), *pos_));

		return *this;
	}

	template <typename O, typename I>
	inline auto filtered_range_iterator_t<O, I>::operator ++(int) -> filtered_range_iterator_t
	{
		auto r = *this;
		++*this;
		return r;
	}

	template <typename O, typename I, typename J>
	inline auto operator == (filtered_range_iterator_t<O, I> const& lhs, filtered_range_iterator_t<O, J> const& rhs) -> bool
	{
		ATMA_ASSERT(lhs.owner() == rhs.owner());
		return lhs.target() == rhs.target();
	}

	template <typename O, typename I, typename J>
	inline auto operator != (filtered_range_iterator_t<O, I> const& lhs, filtered_range_iterator_t<O, J> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}
}


// non-member operators
namespace atma
{
	template <std::ranges::range R, typename F>
	inline auto operator | (R&& range, F&& functor)
	requires detail::is_filter_functor_v<rm_cvref_t<F>>
	{
		if constexpr (detail::is_filtered_range_v<rm_cvref_t<R>>)
		{
			auto predicate = [f=std::forward<R>(range).predicate(), g=std::forward<F>(functor).predicate()](auto&& x) {
				return std::invoke(f, std::forward<decltype(x)>(x)) && std::invoke(g, std::forward<decltype(x)>(x)); };

			return filtered_range_t{range.target_range(), predicate};
		}
		else
		{
			return filtered_range_t{std::forward<R>(range), std::forward<F>(functor).predicate()};
		}
	}

	template <typename F, typename G>
	requires detail::is_filter_functor_v<rm_cvref_t<F>> && detail::is_filter_functor_v<rm_cvref_t<G>>
	inline auto operator * (F&& lhs, G&& rhs)
	{
		auto predicate = [f=lhs.predicate(), g=rhs.predicate()](auto&& x) {
			return std::invoke(f, std::forward<decltype(x)>(x)) && std::invoke(g, std::forward<decltype(x)>(x)); };

		return filter_functor_t{predicate};
	}
}



// non-member functions
namespace atma
{
	// filter: f vs r
	template <typename F, std::ranges::range R>
	requires !detail::is_filtered_range_v<R>
	inline auto filter(F&& predicate, R&& range) {
		return filtered_range_t{std::forward<R>(range), std::forward<F>(predicate)}; }

	// filter: f vs filtered-range<r, g>
	template <typename F, std::ranges::range R>
	requires detail::is_filtered_range_v<R>
	inline auto filter(F&& predicate, R&& range) {
		return std::forward<R>(range) | filter_functor_t{std::forward<F>(predicate)}; }

	// filter: member vs r
	template <typename M, typename C, std::ranges::range R>
	inline auto filter(M C::*m, R&& range)
	{
		auto f = [m](auto&& x) -> bool { return x.*m; };
		return filtered_range_t{std::forward<R>(range), std::move(f)};
	}

	// partial-filter: f
	template <typename F>
	inline auto filter(F&& predicate) {
		return filter_functor_t{predicate}; }
}
