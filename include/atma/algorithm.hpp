#pragma once

#include <algorithm>


namespace atma
{
	template <typename C>
	void sort(C& container)
		{ std::sort(container.begin(), container.end()); }


	template <typename C, typename F>
	void sort(C& container, F&& pred)
		{ std::sort(container.begin(), container.end(), std::forward<F>(f)); }

	template <typename C, typename F> struct filtered_range_t;
	template <typename R> struct filtered_range_iterator_t;




	template <typename C, typename F>
	struct filtered_range_t
	{
		using self_t             = filtered_range_t<C, F>;
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t  = decltype(std::declval<source_container_t>().begin());

		using value_type      = typename source_container_t::value_type;
		using reference       = typename source_container_t::reference;
		using const_reference = typename source_container_t::const_reference;
		using iterator        = filtered_range_iterator_t<self_t>;
		using const_iterator  = filtered_range_iterator_t<self_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;

		// if our source container is const, then even if we are not const, we can only provide const iterators.
		using either_iterator = std::conditional_t<std::is_const<source_container_t>::value, const_iterator, iterator>;


		template <typename CC, typename FF>
		filtered_range_t(CC&&, source_iterator_t const&, source_iterator_t const&, FF&&);
		filtered_range_t(filtered_range_t const&) = delete;

		auto source_container() const -> C { return container_; }
		auto predicate() const -> F { return predicate_; }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> either_iterator;
		auto end() -> either_iterator;

	private:
		C container_;
		source_iterator_t begin_, end_;
		F predicate_;
	};

	template <typename C, typename F>
	auto filter(C&& container, F&& predicate) -> filtered_range_t<C, F>
	{
		return filtered_range_t<C, F>(
			std::forward<C>(container),
			container.begin(),
			container.end(),
			std::forward<F>(predicate));
	}

	template <typename C, typename F>
	template <typename CC, typename FF>
	filtered_range_t<C, F>::filtered_range_t(CC&& source, source_iterator_t const& begin, source_iterator_t const& end, FF&& predicate)
		: container_(std::forward<CC>(source)), begin_(begin), end_(end), predicate_(std::forward<FF>(predicate))
	{
	}


	template <typename C, typename F>
	auto filtered_range_t<C, F>::begin() -> either_iterator
	{
		auto i = begin_;
		while (i != end_ && !predicate_(*i))
			i++;

		return {this, i, end_};
	}

	template <typename C, typename F>
	auto filtered_range_t<C, F>::end() -> either_iterator
	{
		return {this, end_, end_};
	}

	template <typename C, typename F>
	auto filtered_range_t<C, F>::begin() const -> const_iterator
	{
		auto i = begin_;
		while (i != end_ && !predicate_(*i))
			i++;

		return{this, i, end_};
	}

	template <typename C, typename F>
	auto filtered_range_t<C, F>::end() const -> const_iterator
	{
		return{this, end_, end_};
	}


	template <typename C>
	struct filtered_range_iterator_t
	{
		template <typename C2>
		friend auto operator == (filtered_range_iterator_t<C2> const&, filtered_range_iterator_t<C2> const&) -> bool;

		typedef C owner_t;
		typedef typename owner_t::source_iterator_t source_iterator_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;
		using const_reference   = value_type const&;

		filtered_range_iterator_t(C*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() -> std::conditional_t<std::is_const<owner_t>::value, const_reference, reference>;
		auto operator ++() -> filtered_range_iterator_t&;

	private:
		C* owner_;
		source_iterator_t pos_, end_;
	};

	template <typename C>
	inline auto operator == (filtered_range_iterator_t<C> const& lhs, filtered_range_iterator_t<C> const& rhs) -> bool
	{
		ATMA_ASSERT(lhs.owner_ == rhs.owner_);
		return lhs.pos_ == rhs.pos_;
	}

	template <typename C>
	inline auto operator != (filtered_range_iterator_t<C> const& lhs, filtered_range_iterator_t<C> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}



	template <typename C>
	filtered_range_iterator_t<C>::filtered_range_iterator_t(owner_t* owner, source_iterator_t const& begin, source_iterator_t const& end)
		: owner_(owner), pos_(begin), end_(end)
	{
	}

	template <typename C>
	auto filtered_range_iterator_t<C>::operator *() -> std::conditional_t<std::is_const<owner_t>::value, const_reference, reference>
	{
		return *pos_;
	}

	template <typename C>
	auto filtered_range_iterator_t<C>::operator ++() -> filtered_range_iterator_t&
	{
		while (++pos_ != end_ && !owner_->predicate()(*pos_))
			;
		return *this;
	}




#if 0
	template <typename C, typename F>
	struct mapped_range_t
	{
		using self_t = mapped_range_t<C, F>;
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t = typename source_container_t::const_iterator;

		using value_type = typename source_container_t::value_type;
		using reference = typename source_container_t::reference;
		using const_reference = typename source_container_t::const_reference;
		using iterator = mapped_range_iterator_t<self_t>;
		using const_iterator = mapped_range_iterator_t<self_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type = typename source_container_t::size_type;


		mapped_range_t(C, source_iterator_t const&, source_iterator_t const&, F);
		mapped_range_t(mapped_range_t const&) = delete;

		auto source_container() const -> C { return container_; }
		auto fn() const -> F { return fn_; }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;

	private:
		C container_;
		source_iterator_t begin_, end_;
		F fn_;
	};
#endif







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
