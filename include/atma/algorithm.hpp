#pragma once

#include <atma/xtm/function.hpp>

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
	template <typename C, typename F> struct mapped_range_t;
	template <typename R> struct mapped_range_iterator_t;


	//=====================================================================
	//
	//=====================================================================
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

	template <typename F, typename C>
	auto filter(F&& predicate, C&& container) -> filtered_range_t<C, F>
	{
		return filtered_range_t<C, F>(
			std::forward<C>(container),
			container.begin(),
			container.end(),
			std::forward<F>(predicate));
	}

#if 0
	template <typename xs_t, typename c_t, typename mf_t>
	auto filter(c_t (mf_t::*method)(), xs_t&& xs)
	-> filtered_range_t<C, decltype(method)>
	{
		auto lambda = [method](c_t const& x) { return (x.*method)(); };

		return filtered_range_t<xs_t, decltype(lambda)>(
			std::forward<xs_t>(xs),
			container.begin(),
			container.end(),
			lambda);
	}
#endif

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



	//=====================================================================
	//
	//=====================================================================
	template <typename C, typename F>
	struct mapped_range_t
	{
		using self_t             = mapped_range_t<C, F>;
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t  = decltype(std::declval<source_container_t>().begin());

		using value_type      = typename xtm::function_traits<F>::result_type;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using iterator        = mapped_range_iterator_t<self_t>;
		using const_iterator  = mapped_range_iterator_t<self_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;

		// if our source container is const, then even if we are not const, we can only provide const iterators.
		using either_iterator = std::conditional_t<std::is_const<source_container_t>::value, const_iterator, iterator>;


		template <typename CC, typename FF>
		mapped_range_t(CC&&, source_iterator_t const&, source_iterator_t const&, FF&&);
		mapped_range_t(mapped_range_t const&) = delete;

		auto source_container() const -> C { return container_; }
		auto f() const -> F { return fn_; }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> either_iterator;
		auto end() -> either_iterator;

	private:
		C container_;
		source_iterator_t begin_, end_;
		F fn_;
	};

	template <typename C, typename F>
	auto map(F&& f, C&& xs) -> mapped_range_t<C, F>
	{
		return mapped_range_t<C, F>(
			std::forward<C>(xs),
			xs.begin(),
			xs.end(),
			std::forward<F>(f));
	}




	template <typename C, typename F>
	template <typename CC, typename FF>
	mapped_range_t<C, F>::mapped_range_t(CC&& source, source_iterator_t const& begin, source_iterator_t const& end, FF&& f)
		: container_(std::forward<CC>(source)), begin_(begin), end_(end), fn_(std::forward<FF>(f))
	{
	}

	template <typename C, typename F>
	auto mapped_range_t<C, F>::begin() -> either_iterator
	{
		return{this, begin_, end_};
	}

	template <typename C, typename F>
	auto mapped_range_t<C, F>::end() -> either_iterator
	{
		return{this, end_, end_};
	}

	template <typename C, typename F>
	auto mapped_range_t<C, F>::begin() const -> const_iterator
	{
		return{this, begin_, end_};
	}

	template <typename C, typename F>
	auto mapped_range_t<C, F>::end() const -> const_iterator
	{
		return{this, end_, end_};
	}





	template <typename C>
	struct mapped_range_iterator_t
	{
		template <typename C2>
		friend auto operator == (mapped_range_iterator_t<C2> const&, mapped_range_iterator_t<C2> const&) -> bool;

		typedef C owner_t;
		typedef typename owner_t::source_iterator_t source_iterator_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;
		using const_reference   = value_type const&;

		mapped_range_iterator_t(C*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() -> value_type;
		auto operator ++() -> mapped_range_iterator_t&;

	private:
		C* owner_;
		source_iterator_t pos_, end_;
	};

	template <typename C>
	inline auto operator == (mapped_range_iterator_t<C> const& lhs, mapped_range_iterator_t<C> const& rhs) -> bool
	{
		ATMA_ASSERT(lhs.owner_ == rhs.owner_);
		return lhs.pos_ == rhs.pos_;
	}

	template <typename C>
	inline auto operator != (mapped_range_iterator_t<C> const& lhs, mapped_range_iterator_t<C> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}



	template <typename C>
	mapped_range_iterator_t<C>::mapped_range_iterator_t(owner_t* owner, source_iterator_t const& begin, source_iterator_t const& end)
		: owner_(owner), pos_(begin), end_(end)
	{
	}

	template <typename C>
	auto mapped_range_iterator_t<C>::operator *() -> value_type
	{
		return owner_->f()(*pos_);
	}

	template <typename C>
	auto mapped_range_iterator_t<C>::operator ++() -> mapped_range_iterator_t&
	{
		++pos_;
		return *this;
	}



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

	template <typename xs_t>
	auto slice(xs_t&& xs, uint begin, uint stop) -> range_t<xs_t>
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


	
#if 0
	template <typename source_t>
	inline auto make_vector(source_t&& source)
		-> std::vector<typename std::remove_reference<source_t>::type::value_type>
	{
		return{source.begin(), source.end()};
	}

	template <size_t E, typename R, typename V, typename F, typename... FS>
	auto count_all_impl(R& result, V const& value, F&& fn1, FS&&... fns) -> void
	{
		if (fn1(value))
			++std::get<E>(result);

		count_all_impl<E + 1>(result, value, fns...);
	}

	template <size_t E, typename R, typename V>
	auto count_all_impl(R& result, V const& value) -> void
	{
	}

	template <typename xs_t, typename... fns_t>
	auto count_all(xs_t&& xs, fns_t&&... fns) -> atma::xtm::tuple_integral_list_t<uint, sizeof...(fns_t)>
	{
		atma::xtm::tuple_integral_list_t<uint, sizeof...(fns_t)> result;

		for (auto const& x : xs)
		{
			count_all_impl<0>(result, x, fns...);
		}

		return result;
	}

	template <typename xs_t, typename f_t>
	auto all_of(xs_t&& xs, f_t&& f)
		-> typename std::enable_if<std::is_same<typename atma::xtm::function_traits<f_t>::result_type, bool>::value, bool>::type
	{
		for (auto const& x : xs)
			if (!f(x))
				return false;

		return true;
	}

	template <typename xs_t, typename c_t, typename m_t>
	auto copy_if(xs_t&& xs, m_t c_t::*member)
		-> std::enable_if_t<std::is_same<typename std::decay<xs_t>::type::value_type, c_t>::value, std::decay_t<xs_t>>
	{
		std::decay_t<xs_t> result;

		for (auto&& x : xs)
		{
			if (x.*member)
				result.push_back(x);
		}

		return result;
	}

	template <typename xs_t, typename c_t, typename m_t>
	auto copy_if(xs_t&& xs, m_t(c_t::*member)())
		-> std::enable_if_t<std::is_same<typename std::decay<xs_t>::type::value_type, c_t>::value, std::decay_t<xs_t>>
	{
		std::decay_t<xs_t> result;

		for (auto&& x : xs)
		{
			if ((x.*member)())
				result.push_back(x);
		}

		return result;
	}

	template <typename xs_t, typename c_t, typename m_t>
	auto copy_if(xs_t&& xs, m_t(c_t::*member)() const)
		-> std::enable_if_t<std::is_same<typename std::decay<xs_t>::type::value_type, c_t>::value, std::decay_t<xs_t>>
	{
		std::decay_t<xs_t> result;

		for (auto&& x : xs)
		{
			if ((x.*member)())
				result.push_back(x);
		}

		return result;
	}

#endif
	
	template <typename xs_t, typename f_t>
	inline auto for_each(xs_t&& xs, f_t&& f) -> void
	{
		for (auto&& x : xs)
			f(x);
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
