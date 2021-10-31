#pragma once

#include <atma/function.hpp>
#include <atma/ranges/core.hpp>

import atma.types;

// forward-declares
namespace atma
{
	template <typename, typename> struct mapped_range_t;
	template <typename> struct mapped_range_iterator_t;
	template <typename> struct map_functor_t;
}


// is_mapped_range
namespace atma::detail
{
	template <typename T>
	struct is_mapped_range : std::false_type
	{};

	template <typename R, typename F>
	struct is_mapped_range<mapped_range_t<R, F>> : std::true_type
	{};

	template <typename T>
	inline constexpr bool is_mapped_range_v = is_mapped_range<T>::value;
}


// is_map_functor
namespace atma::detail
{
	template <typename T>
	struct is_map_functor : std::false_type
	{};

	template <typename F>
	struct is_map_functor<map_functor_t<F>> : std::true_type
	{};

	template <typename T>
	inline constexpr bool is_map_functor_v = is_map_functor<T>::value;
}


// mapped-range
namespace atma
{
	template <typename T>
	decltype(auto) fwd(T&& t) { return std::forward<T>(t); }

	// R: either a prvalue, or a reference (mutable or const)
	template <typename R, typename F>
	struct mapped_range_t
	{
		using self_t          = mapped_range_t<R, F>;
		using target_range_t  = std::remove_reference_t<R>;

		using storage_range_t     = storage_type_t<R>;
		using storage_functor_t   = storage_type_t<F>;
		using target_iterator_t   = decltype(std::declval<storage_range_t>().begin());
		using invoke_result_t     = std::invoke_result_t<F, typename target_range_t::value_type>;

		// the value-type of the what F returns. we must remove references
		using value_type      = std::remove_reference_t<invoke_result_t>;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using iterator        = mapped_range_iterator_t<transfer_const_t<target_range_t, self_t>>;
		using const_iterator  = mapped_range_iterator_t<self_t const>;
		using difference_type = typename target_range_t::difference_type;
		using size_type       = typename target_range_t::size_type;


		mapped_range_t(mapped_range_t const&) = default;
		mapped_range_t(mapped_range_t&&) = default;

		template <typename RR, typename FF>
		mapped_range_t(RR&&, FF&&);

		decltype(auto) target_range() &  { return (range_); }
		decltype(auto) target_range() && { return std::forward<storage_range_t>(range_); }
		decltype(auto) function() &  { return (fn_); }
		decltype(auto) function() && { return std::forward<F>(fn_); }
		
		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

	private:
		storage_range_t range_;
		storage_functor_t fn_;

		template <typename Owner>
		friend struct mapped_range_iterator_t;
	};

	template <typename R, typename F>
	mapped_range_t(R&& r, F&& f) -> mapped_range_t<R, F>;
}


// map-functor
namespace atma
{
	template <typename F>
	struct map_functor_t
	{
		template <typename FF>
		map_functor_t(FF&& f)
			: fn_(std::forward<FF>(f))
		{}

		template <typename R>
		auto operator ()(R&& xs) {
			return mapped_range_t{std::forward<R>(xs), function()};
		}

		template <typename R>
		auto operator ()(R&& xs) const {
			return mapped_range_t{std::forward<R>(xs), function()};
		}

		decltype(auto) function() &  { return (fn_); }
		decltype(auto) function() && { return std::forward<storage_type_t<F>>(fn_); }

	private:
		storage_type_t<F> fn_;
	};

	template <typename F>
	map_functor_t(F&& f) -> map_functor_t<F>;
}


// mapped-range-iterator
namespace atma
{
	template <typename R>
	struct mapped_range_iterator_t
	{
		template <typename C2>
		friend auto operator == (mapped_range_iterator_t<C2> const&, mapped_range_iterator_t<C2> const&) -> bool;

		using owner_t           = R;
		using target_iterator_t = typename std::remove_reference_t<owner_t>::target_iterator_t;
		using invoke_result_t   = typename owner_t::invoke_result_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = std::remove_reference_t<invoke_result_t>;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = invoke_result_t;

		mapped_range_iterator_t();
		mapped_range_iterator_t(R*, target_iterator_t const& begin, target_iterator_t const& end);

		auto operator = (mapped_range_iterator_t const& rhs) -> mapped_range_iterator_t&
		{
			owner_ = rhs.owner_;
			pos_ = rhs.pos_;
			end_ = rhs.end_;
			return *this;
		}

		auto operator  *() const -> invoke_result_t;
		auto operator ++() -> mapped_range_iterator_t&;
		auto operator ++(int) -> mapped_range_iterator_t;

	private:
		R* owner_;
		target_iterator_t pos_, end_;
	};




	//======================================================================
	// RANGE IMPLEMENTATION
	//======================================================================
	template <typename R, typename F>
	template <typename RR, typename FF>
	inline mapped_range_t<R, F>::mapped_range_t(RR&& target, FF&& f)
		: range_(std::forward<RR>(target))
		, fn_(std::forward<FF>(f))
	{}

	template <typename R, typename F>
	inline auto mapped_range_t<R, F>::begin() -> iterator
	{
		return iterator(this, range_.begin(), range_.end());
	}

	template <typename R, typename F>
	inline auto mapped_range_t<R, F>::end() -> iterator
	{
		return iterator(this, range_.end(), range_.end());
	}

	template <typename R, typename F>
	inline auto mapped_range_t<R, F>::begin() const -> const_iterator
	{
		return const_iterator{this, range_.begin(), range_.end()};
	}

	template <typename R, typename F>
	inline auto mapped_range_t<R, F>::end() const -> const_iterator
	{
		return const_iterator{this, range_.end(), range_.end()};
	}




	//======================================================================
	// ITERATOR IMPLEMENTATION
	//======================================================================
	template <typename R>
	inline mapped_range_iterator_t<R>::mapped_range_iterator_t()
		: owner_()
		, pos_()
		, end_()
	{}

	template <typename R>
	inline mapped_range_iterator_t<R>::mapped_range_iterator_t(owner_t* owner, target_iterator_t const& begin, target_iterator_t const& end)
		: owner_(owner), pos_(begin), end_(end)
	{}

	template <typename R>
	inline auto mapped_range_iterator_t<R>::operator *() const -> invoke_result_t
	{
		return range_function_invoke(owner_->fn_, *pos_);
	}

	template <typename R>
	inline auto mapped_range_iterator_t<R>::operator ++() -> mapped_range_iterator_t&
	{
		++pos_;
		return *this;
	}

	template <typename R>
	inline auto mapped_range_iterator_t<R>::operator ++(int) -> mapped_range_iterator_t
	{
		auto r = *this;
		++*this;
		return r;
	}

	template <typename R>
	inline auto operator == (mapped_range_iterator_t<R> const& lhs, mapped_range_iterator_t<R> const& rhs) -> bool
	{
		ATMA_ASSERT(lhs.owner_ == rhs.owner_);
		return lhs.pos_ == rhs.pos_;
	}

	template <typename R>
	inline auto operator != (mapped_range_iterator_t<R> const& lhs, mapped_range_iterator_t<R> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}


	//======================================================================
	// operators
	//======================================================================
	template <std::ranges::range R, typename F>
	requires detail::is_map_functor_v<rm_cvref_t<F>>
	inline auto operator | (R&& range, F&& functor)
	{
		if constexpr (detail::is_mapped_range_v<rm_cvref_t<R>>)
		{
			auto function = [f=std::forward<R>(range).function(), g=std::forward<F>(functor).function()](auto&& x) {
				return std::invoke(g, std::invoke(f, std::forward<decltype(x)>(x))); };

			return mapped_range_t{std::forward<R>(range).target_range(), function};
		}
		else
		{
			return mapped_range_t{std::forward<R>(range), std::forward<F>(functor).function()};
		}
	}


	//======================================================================
	// FUNCTIONS
	//======================================================================
	template <std::ranges::range R, typename F>
	inline auto map(F f, R&& range)
	{
		return mapped_range_t{std::forward<R>(range), std::forward<F>(f)};
	}

	template <typename F>
	inline auto map(F&& f)
	{
		return map_functor_t{std::forward<F>(f)};
	}

	template <typename M, typename R>
	inline auto map(M R::*member)
	{
		return map_functor_t{[member](auto&& x) { return x.*member; }};
	}

}

