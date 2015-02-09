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


	template <typename... Ranges>
	struct zip_range_iterator_t
	{
		using range_tuple_type = std::tuple<Ranges...>;
		using value_type = std::tuple<typename std::remove_reference<Ranges>::type::value_type...>;
		using reference = std::tuple<typename std::remove_reference<Ranges>::type::reference...>;

		using internal_iter_tuple = std::tuple<typename detail::zip_internal_range_iterator_t<Ranges>::type...>;

		zip_range_iterator_t(internal_iter_tuple const& iters)
			: iters_{iters}
		{
		}

		auto operator ++ () -> zip_range_iterator_t&
		{
			atma::tuple_apply<atma::increment_adaptor_t>(iters_);
			return *this;
		}

		auto operator *() -> value_type
		{
			return atma::tuple_apply<atma::dereference_adaptor_t>(iters_);
		}

	private:
		internal_iter_tuple iters_;

		template <typename... Ranges>
		friend auto operator == (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool;
	};


	template <typename... Ranges>
	auto operator == (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		// equality is defined whenever *any* of our internal iterators matches another.
		// since some zipped ranges might have different lengths, we'll hit the "end" iterator
		// earlier on some ranges. these will then be equal, but other ranges might still have valid
		// iterators not pointing to the end-iterator. but this is the terminating point for zip, so
		// we evaluate to equal, and end iteration.
		//
		// example shown. a zip of two ranges of differing lengths, showing where current iteration
		// is, and where the "end" iterator of the zip-range has been constructed:
		//
		//    internal range 1:   {1, 2, 3, 4} end.
		//    internal iter at:                ^
		//    end iter at:                     ^
		//
		//    internal range 2:   {a, b, c, d, e, f} end.
		//    internal iter at:                ^
		//    end iter at:                           ^
		//
		// this demonstrates why zip-range-iterator equality is defined as when *any* of the internal
		// iterators match. because otherwise we'd traverse past the end of some of them.
		//
		return atma::tuple_any_elem_eq(lhs.iters_, rhs.iters_);
	}

	template <typename... Ranges>
	auto operator != (zip_range_iterator_t<Ranges...> const& lhs, zip_range_iterator_t<Ranges...> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}


	template <typename... Ranges>
	struct zip_range_t
	{
		zip_range_t(Ranges... ranges)
			: ranges_{ranges...}
		{
		}

		auto begin() -> zip_range_iterator_t<Ranges...>
		{
			return{atma::tuple_apply<atma::begin_adaptor_t>(ranges_)};
		}

		auto end() -> zip_range_iterator_t<Ranges...>
		{
			return{atma::tuple_apply<atma::end_adaptor_t>(ranges_)};
		}

	private:
		std::tuple<Ranges...> ranges_;

	};


	template <typename... Ranges>
	inline auto zip(Ranges&&... ranges) -> zip_range_t<Ranges...>
	{
		return {std::forward<Ranges>(ranges)...};
	}
}
