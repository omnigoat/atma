#pragma once

#include <atma/tuple.hpp>


namespace atma
{
	template <typename...> struct zip_range_t;
	template <typename...> struct zip_range_iterator_t;
	template <typename...> struct zip_range_sentinel_t;
}


// specialize std::common_type
namespace std
{
	template <typename... Ranges>
	struct common_type<atma::zip_range_iterator_t<Ranges...>, atma::zip_range_sentinel_t<Ranges...>>
	{
		using type = atma::zip_range_iterator_t<Ranges...>;
	};
}


// zip_range_t
namespace atma
{
	template <typename... Ranges>
	struct zip_range_t
	{
		using value_type      = std::tuple<typename std::remove_reference_t<Ranges>::value_type...>;
		using reference       = std::tuple<typename std::remove_reference_t<Ranges>::reference...>;
		using const_reference = std::tuple<typename std::remove_reference_t<Ranges>::const_reference...>;
		using iterator        = zip_range_iterator_t<Ranges...>;
		using const_iterator  = zip_range_iterator_t<Ranges const...>;
		using difference_type = ptrdiff_t;
		using size_type       = size_t;

		zip_range_t(Ranges... ranges);

		auto begin() -> iterator;
		auto end() -> zip_range_sentinel_t<Ranges...>;
		auto begin() const -> const_iterator;
		auto end() const -> zip_range_sentinel_t<Ranges...>;

	private:
		std::tuple<Ranges...> ranges_;
	};

	template <typename... Ranges>
	zip_range_t(Ranges&&... ranges) -> zip_range_t<Ranges...>;

}

// zip_range_iterator_t
namespace atma
{
	template <typename T>
	struct membertype_value_type {
		using type = typename std::remove_reference_t<T>::value_type;
	};

	template <typename T>
	struct membertype_reference {
		using type = typename std::remove_reference_t<T>::reference;
	};

	template <typename... Ranges>
	struct zip_range_iterator_t<Ranges...>
	{
		using range_tuple_type = std::tuple<Ranges...>;
		using target_iters_tuple_t = std::tuple<decltype(std::begin(std::declval<Ranges&>()))...>;

		// concept: Iterator
		using difference_type   = ptrdiff_t;
		using value_type        = tuple_map_t<membertype_value_type, range_tuple_type>;
		using pointer           = value_type*;
		using reference         = std::tuple<typename std::remove_reference_t<Ranges>::reference...>;
		using iterator_category = std::forward_iterator_tag;

		zip_range_iterator_t();
		zip_range_iterator_t(target_iters_tuple_t const&);

		auto operator = (zip_range_iterator_t const&) -> zip_range_iterator_t&;

		auto operator  *() const -> reference;
		auto operator ++() -> zip_range_iterator_t&;
		auto operator ++(int) -> zip_range_iterator_t;

		auto base() const -> target_iters_tuple_t const& { return iters_; }
		auto base() -> target_iters_tuple_t& { return iters_; }

	private:
		target_iters_tuple_t iters_;
	};
}


// zip_range_sentinel_t
namespace atma
{
	template <typename... Ranges>
	struct zip_range_sentinel_t : zip_range_iterator_t<Ranges...>
	{
		using zip_range_iterator_t<Ranges...>::zip_range_iterator_t;
	};
}


//======================================================================
// RANGE IMPLEMENTATION
//======================================================================
namespace atma
{
	template <typename... Ranges>
	zip_range_t<Ranges...>::zip_range_t(Ranges... ranges)
		: ranges_{ranges...}
	{}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::begin() -> iterator
	{
		return {atma::tuple_apply(atma::begin_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::end() -> zip_range_sentinel_t<Ranges...>
	{
		return zip_range_sentinel_t<Ranges...>{atma::tuple_apply(atma::end_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::begin() const -> const_iterator
	{
		return {atma::tuple_apply(atma::begin_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::end() const -> zip_range_sentinel_t<Ranges...>
	{
		return zip_range_sentinel_t<Ranges...>{atma::tuple_apply(atma::end_functor_t(), ranges_)};
	}
}


//======================================================================
// ITERATOR IMPLEMENTATION
//======================================================================
namespace atma
{
	template <typename... Ranges>
	zip_range_iterator_t<Ranges...>::zip_range_iterator_t()
	{}

	template <typename... Ranges>
	zip_range_iterator_t<Ranges...>::zip_range_iterator_t(target_iters_tuple_t const& iters)
		: iters_{iters}
	{}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator = (zip_range_iterator_t const& rhs) -> zip_range_iterator_t&
	{
		atma::tuple_binary_apply(assign_functor_t(), base(), rhs.base());
		return *this;
	}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator ++ () -> zip_range_iterator_t&
	{
		atma::tuple_apply(atma::increment_functor_t(), base());
		return *this;
	}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator ++ (int) -> zip_range_iterator_t
	{
		auto r = *this;
		++*this;
		return r;
	}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator *() const -> reference
	{
		return tuple_apply(atma::dereference_functor_t{}, base());
	}

	template <typename... Ranges>
	inline auto operator == (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return atma::tuple_all_elem_eq(lhs.base(), rhs.base());
	}

	template <typename... Ranges>
	inline auto operator != (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	// an iterator equals the sentinel the moment any of its internal iterators are EOF
	template <typename... Ranges>
	inline auto operator == (zip_range_iterator_t<Ranges...> const& lhs, zip_range_sentinel_t<Ranges...> const& rhs) -> bool
	{
		return atma::tuple_any_elem_eq(lhs.base(), rhs.base());
	}

	template <typename... Ranges>
	inline auto operator != (zip_range_iterator_t<Ranges...> const& lhs, zip_range_sentinel_t<Ranges...> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	template <typename... Ranges>
	inline auto operator == (zip_range_sentinel_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return operator == (rhs, lhs);
	}

	template <typename... Ranges>
	inline auto operator != (zip_range_sentinel_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return !operator == (rhs, lhs);
	}
}


//======================================================================
// NON-MEMBER FUNCTIONS
//======================================================================
namespace atma
{
	template <std::ranges::range... Ranges>
	inline auto zip(Ranges&&... ranges)
	{
		return zip_range_t{std::forward<Ranges>(ranges)...};
	}
}
