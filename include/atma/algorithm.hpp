#ifndef ATMA_ALGORITHM_HPP
#define ATMA_ALGORITHM_HPP
//=====================================================================
namespace atma {
//=====================================================================
	
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

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================