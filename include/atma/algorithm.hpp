#pragma once

#include <atma/function_traits.hpp>
#include <atma/tuple.hpp>
#include <atma/ranges/filter.hpp>
#include <atma/ranges/map.hpp>
#include <atma/ranges/zip.hpp>

#include <algorithm>


namespace atma
{
	// reimplement many std algorithms
	template <typename C>
	void sort(C& container)
		{ std::sort(container.begin(), container.end()); }

	template <typename C, typename F>
	void sort(C& container, F&& pred)
		{ std::sort(container.begin(), container.end(), std::forward<F>(f)); }



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


	constexpr inline struct as_vector_t
	{
		template <typename R>
		auto operator | (R&& range) const
		{
			return atma::vector<typename std::remove_reference_t<R>::value_type>{range.begin(), range.end()};
		}
	} as_vector;

	template <typename R>
	inline auto operator | (R&& range, as_vector_t const&)
	{
		return atma::vector<typename std::remove_reference_t<R>::value_type>{range.begin(), range.end()};
	}



	//=====================================================================
	// all_of
	//=====================================================================
	template <typename XS, typename F>
	auto all_of(XS&& xs, F&& f) -> bool
	{
		for (auto const& x : xs)
			if (!f(x))
				return false;

		return true;
	}

	//=====================================================================
	// copy_if
	//=====================================================================
	template <typename XS, typename C, typename M>
	auto copy_if(XS&& xs, M C::*member) -> std::decay_t<XS>
	{
		using DXS = std::decay_t<XS>;

		static_assert(std::is_same<DXS::value_type, C>::value, "member-pointer's class doesn't match range's value-type");

		DXS result;

		for (auto&& x : xs)
		{
			if (x.*member)
				result.push_back(x);
		}

		return result;
	}

	template <typename XS, typename C, typename M>
	auto copy_if(XS&& xs, M(C::*method)()) -> std::decay_t<XS>
	{
		using DXS = std::decay_t<XS>;
		static_assert(std::is_same<DXS::value_type, C>::value, "method-pointer's class doesn't match range's value-type");
		DXS result;

		for (auto&& x : xs)
		{
			if ((x.*method)())
				result.push_back(x);
		}

		return result;
	}

	template <typename XS, typename C, typename M>
	auto copy_if(XS&& xs, M(C::*method)() const) -> std::decay_t<XS>
	{
		using DXS = std::decay_t<XS>;
		static_assert(std::is_same<DXS::value_type, C>::value, "method-pointer's class doesn't match range's value-type");
		DXS result;

		for (auto&& x : xs)
		{
			if ((x.*method)())
				result.push_back(x);
		}

		return result;
	}

	//=====================================================================
	// remove_erase
	//=====================================================================
	template <typename range_tx, typename pred_tx>
	auto remove_erase(range_tx& range, pred_tx const& pred) -> void
	{
		range.erase(
			std::remove_if(range.begin(), range.end(), pred),
			range.end());
	}


	//=====================================================================
	// for_each / for_each2
	//=====================================================================
	template <typename XS, typename F>
	inline auto for_each(XS&& xs, F&& f) -> void
	{
		for (auto&& x : xs)
			f(x);
	}

	template <typename IT, typename F>
	inline auto for_each(IT const& begin, IT const& end, F&& f) -> void
	{
		for (auto i = begin; i != end; ++i)
			f(*i);
	}

	template <typename R, typename LHS, typename F>
	inline auto for_each2(R&& range, LHS&& lhs, F&& fn) -> void
	{
		for (auto&& x : range)
			fn(std::forward<LHS>(lhs), x);
	}

	template <typename IT, typename LHS, typename F>
	inline auto for_each2(IT const& begin, IT const& end, LHS&& lhs, F&& fn) -> void
	{
		for (auto i = begin; i != end; ++i)
			fn(std::forward<LHS>(lhs), *i);
	}

	//=====================================================================
	// find_in
	//=====================================================================
	template <typename R>
	inline auto find_in(R&& range, typename std::remove_reference_t<R>::value_type const& x)
	{
		return std::find(std::begin(range), std::end(range), x);
	}


	//=====================================================================
	// foldl
	//=====================================================================
	template <typename R, typename F>
	inline auto foldl(R&& range, F&& fn) -> typename std::decay_t<R>::value_type
	{
		ATMA_ASSERT(!range.empty());

		auto r = range.first();

		for (auto i = ++range.begin(); i != range.end(); ++i)
			r = fn(r, *i);
		
		return r;
	}

	template <typename R, typename I, typename F>
	inline auto foldl(R&& range, I&& initial, F&& fn) -> std::decay_t<I>
	{
		ATMA_ASSERT(!range.empty());

		auto r = initial;

		for (auto i = range.begin(); i != range.end(); ++i)
			r = fn(r, *i);

		return r;
	}

	template <typename IT, typename I, typename F>
	inline auto foldl(IT const& begin, IT const& end, I&& initial, F&& fn) -> std::decay_t<I>
	{
		ATMA_ASSERT(!range.empty());

		auto r = initial;

		for (auto i = begin; i != end; ++i)
			r = fn(r, *i);

		return r;
	}

	

	//=====================================================================
	// merge
	// -------
	//   takes two ranges, a less-than predicate, merging function, and two functions
	//   for when merging fails, and merges equivalent elements together (using the
	//   merging function), and outputs them into an output iterator. elements not
	//   merged are passed to their respective merging function (one for each range)
	//
	//   returns the output iterator
	//=====================================================================
	template <typename LIT, typename RIT, typename OT, typename MERGER, typename LFN, typename RFN, typename PR>
	OT merge(LIT xs_begin, LIT xs_end, RIT ys_begin, RIT ys_end, OT out, MERGER merger, LFN lfn, RFN rfn, PR pred)
	{
		static_assert(std::is_same<typename LIT::value_type, typename RIT::value_type>::value == true, "value-types different");

		auto x = xs_begin;
		auto y = ys_begin;
		while (x != xs_end && y != ys_end) {
			if (pred(*x, *y)) {
				lfn(*x);
				++x;
			}
			else if (pred(*y, *x)) {
				rfn(*y);
				++y;
			}
			else {
				*out++ = merger(*x, *y);
				++x; ++y;
			}
		}

		while (x != xs_end)
			lfn(*x++);

		while (y != ys_end)
			rfn(*y++);

		return out;
	}

	// default predicate
	template <typename LIT, typename RIT, typename OT, typename MERGER, typename LFN, typename RFN>
	OT merge(LIT xs_begin, LIT xs_end, RIT ys_begin, RIT ys_end, OT out, MERGER merger, LFN lfn, RFN rfn)
	{
		return merge(xs_begin, xs_end, ys_begin, ys_end, out, merger, lfn, rfn, std::less<typename LIT::value_type>());
	}

	// do-nothing failure-function
	template <typename LIT, typename RIT, typename OT, typename MERGER, typename PR>
	OT merge(LIT xs_begin, LIT xs_end, RIT ys_begin, RIT ys_end, OT out, MERGER merger, PR pred)
	{
		auto do_nothing = [](typename LIT::value_type const&){};
		return merge(xs_begin, xs_end, ys_begin, ys_end, out, merger, do_nothing, do_nothing, pred);
	}

	// do-nothing failure-function, default-predicate
	template <typename LIT, typename RIT, typename OT, typename MERGER>
	OT merge(LIT xs_begin, LIT xs_end, RIT ys_begin, RIT ys_end, OT out, MERGER merger)
	{
		auto do_nothing = [](typename LIT::value_type const&){};
		return merge(xs_begin, xs_end, ys_begin, ys_end, out, merger, do_nothing, do_nothing, std::less<typename LIT::value_type>());
	}



	//=====================================================================
	//=====================================================================
	template <typename IT, typename OTS, typename OTF, typename PR>
	void seperate(IT begin, IT end, OTS out_succeed, OTF out_failure, PR pred) {
		while (begin != end) {
			if (pred(*begin)) {
				*out_succeed++ = *begin;
			}
			else {
				*out_failure++ = *begin;
			}
			++begin;
		}
	}
}
