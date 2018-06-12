#pragma once

#include <atma/assert.hpp>
#include <atma/concepts.hpp>
#include <atma/ranges/core.hpp>


namespace atma
{
	template <typename R, typename F> struct filtered_range_t;
	template <typename R> struct filtered_range_iterator_t;
	template <typename F> struct filter_functor_t;
}


// is_filtered_range
namespace atma::detail
{
	template <typename T>
	struct is_filtered_range
	{
		static constexpr bool value = false;
	};

	template <typename R, typename F>
	struct is_filtered_range<filtered_range_t<R, F>>
	{
		static constexpr bool value = true;
	};

	template <typename T>
	inline constexpr bool is_filtered_range_v = is_filtered_range<T>::value;
}


// is_filter_functor
namespace atma::detail
{
	template <typename T>
	struct is_filter_functor
	{
		static constexpr bool value = false;
	};

	template <typename F>
	struct is_filter_functor<filter_functor_t<F>>
	{
		static constexpr bool value = true;
	};

	template <typename T>
	inline constexpr bool is_filter_functor_v = is_filter_functor<T>::value;
}



// filtered-range
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

		template <typename RR, typename FF>
		filtered_range_t(RR&&, FF&&);

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
		decltype(auto) operator ()(R&& xs) const &
		{
			return filtered_range_t{std::forward<R>(xs), fn_};
		}

		template <typename R>
		decltype(auto) operator ()(R&& xs) const &&
		{
			return filtered_range_t{std::forward<R>(xs), std::forward<storage_functor_t>(fn_)};
		}

		decltype(auto) predicate() const { return std::forward<storage_functor_t>(fn_); }
		decltype(auto) predicate()       { return std::forward<storage_functor_t>(fn_); }

	private:
		storage_functor_t fn_;
	};

	template <typename F>
	filter_functor_t(F&& f) -> filter_functor_t<F>;
}


// filtered-range-iterator
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


// non-member operators
namespace atma
{
#if 0
	template <typename F, typename R,
		CONCEPT_REQUIRES_(
			detail::is_filter_functor_v<remove_cvref_t<F>>,
			is_range_v<remove_cvref_t<R>>)>
	inline auto operator | (F&& lhs, R&& rhs)
	{
		if constexpr (detail::is_filtered_range_v<remove_cvref_t<R>>)
		{
			auto predicate = [f=lhs.predicate(), g=rhs.predicate()](auto&& x) {
				return g(std::forward<decltype(x)>(x)) && f(std::forward<decltype(x)>(x)); };

			return filtered_range_t{rhs.source_container(), predicate};
		}
		else
		{
			return filtered_range_t{std::forward<R>(rhs), lhs.predicate()};
		}
	}
#endif

	template <typename R, typename F,
		CONCEPT_REQUIRES_(
			is_range_v<remove_cvref_t<R>>,
			detail::is_filter_functor_v<remove_cvref_t<F>>)>
	inline auto operator | (R&& lhs, F&& rhs)
	{
		if constexpr (detail::is_filtered_range_v<remove_cvref_t<R>>)
		{
			auto predicate = [f=lhs.predicate(), g=rhs.predicate()](auto&& x) {
				return std::invoke(f, std::forward<decltype(x)>(x)) && std::invoke(g, std::forward<decltype(x)>(x)); };

			return filtered_range_t{lhs.source_container(), predicate};
		}
		else
		{
			return filtered_range_t{std::forward<R>(lhs), rhs.predicate()};
		}
	}

	template <typename F, typename G,
		CONCEPT_REQUIRES_(
			detail::is_filter_functor_v<remove_cvref_t<F>>,
			detail::is_filter_functor_v<remove_cvref_t<G>>)>
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
	template <typename F, typename R,
		CONCEPT_REQUIRES_(
			is_range_v<R>,
			!detail::is_filtered_range_v<R>)>
	inline auto filter(F&& predicate, R&& container) {
		return filtered_range_t{std::forward<R>(container), std::forward<F>(predicate)}; }

	// filter: f vs filtered-range<r, g>
	template <typename F, typename R,
		CONCEPT_REQUIRES_(detail::is_filtered_range_v<R>)>
	inline auto filter(F&& predicate, R&& range) {
		return std::forward<R>(range) | filter_functor_t{std::forward<F>(predicate)}; }

	// filter: member vs r
	template <typename M, typename C, typename R,
		CONCEPT_REQUIRES_(is_range_v<R>)>
	inline auto filter(M C::*m, R&& range)
	{
		auto f = [m](auto&& x) -> bool { return x.*m; };
		return filtered_range_t{std::forward<R>(range), f};
	}

	// partial-filter: f
	template <typename F>
	inline auto filter(F&& predicate) {
		return filter_functor_t{predicate}; }
}
