#pragma once

#include <atma/tuple.hpp>

namespace atma
{
	namespace detail
	{
		template <typename C>
		struct zip_internal_range_iterator_t
		{
			using C2 = std::remove_reference_t<C>;

			using type = std::conditional_t<std::is_const<C2>::value, typename C2::const_iterator, typename C2::iterator>;
		};
	}

	template <typename...> struct zip_range_t;
	template <typename...> struct zip_range_iterator_t;


	template <typename... Ranges>
	struct zip_range_t
	{
		using value_type      = std::tuple<typename std::remove_reference<Ranges>::type::value_type...>;
		using reference       = std::tuple<typename std::remove_reference<Ranges>::type::reference...>;
		using const_reference = std::tuple<typename std::remove_reference<Ranges>::type::const_reference...>;
		using iterator        = zip_range_iterator_t<Ranges...>;
		using const_iterator  = zip_range_iterator_t<Ranges const...>;

		// seriously: what.
		using difference_type = ptrdiff_t;
		using size_type       = size_t;


		zip_range_t(Ranges... ranges);

		auto begin() -> iterator;
		auto end() -> iterator;
		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;

	private:
		std::tuple<Ranges...> ranges_;
	};



	template <typename... Ranges>
	struct zip_range_iterator_t
	{
		using range_tuple_type = std::tuple<Ranges...>;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = std::tuple<typename std::remove_reference<Ranges>::type::value_type...>;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = std::tuple<typename std::remove_reference<Ranges>::type::reference...>;


		using internal_iters_tuple_t = std::tuple<typename detail::zip_internal_range_iterator_t<Ranges>::type...>;

		zip_range_iterator_t(internal_iters_tuple_t const& iters);

		auto operator ++() -> zip_range_iterator_t&;
		auto operator  *() -> reference;
		auto operator  *() const -> reference const;

	private:
		internal_iters_tuple_t iters_;

		template <typename... Ranges>
		friend auto operator == (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool;
	};




	//======================================================================
	// RANGE IMPLEMENTATION
	//======================================================================
	template <typename... Ranges>
	zip_range_t<Ranges...>::zip_range_t(Ranges... ranges)
		: ranges_{ranges...}
	{
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::begin() -> iterator
	{
		return {atma::tuple_apply(atma::begin_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::end() -> iterator
	{
		return {atma::tuple_apply(atma::end_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::begin() const -> const_iterator
	{
		return {atma::tuple_apply(atma::begin_functor_t(), ranges_)};
	}

	template <typename... Ranges>
	auto zip_range_t<Ranges...>::end() const -> const_iterator
	{
		return {atma::tuple_apply(atma::end_functor_t(), ranges_)};
	}



	//======================================================================
	// ITERATOR IMPLEMENTATION
	//======================================================================
	template <typename... Ranges>
	zip_range_iterator_t<Ranges...>::zip_range_iterator_t(internal_iters_tuple_t const& iters)
		: iters_{iters}
	{
	}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator ++ () -> zip_range_iterator_t&
	{
		atma::tuple_apply(atma::increment_functor_t(), iters_);
		return *this;
	}

	template <typename... Ranges>
	auto zip_range_iterator_t<Ranges...>::operator *() -> reference
	{
		return atma::tuple_apply(atma::dereference_functor_t(), iters_);
	}

	template <typename... Ranges>
	auto operator != (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return atma::tuple_any_elem_neq(lhs.iters_, rhs.iters_);
	}




	template <typename... Ranges>
	inline auto zip(Ranges&&... ranges) -> zip_range_t<Ranges...>
	{
		return {std::forward<Ranges>(ranges)...};
	}
}
