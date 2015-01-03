#pragma once

namespace atma
{
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
		filtered_range_t(filtered_range_t const&);

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




	template <typename F>
	struct partial_filtered_range_t
	{
		template <typename FF>
		partial_filtered_range_t(FF&& f)
			: fn_(std::forward<FF>(f))
		{}

		template <typename C>
		inline auto operator <<= (C&& xs) -> filtered_range_t<C, F>
		{
			return filtered_range_t<C, F>(
				std::forward<C>(xs),
				xs.begin(),
				xs.end(),
				std::forward<F>(fn_));
		}

	private:
		F fn_;
	};



	template <typename C>
	struct filtered_range_iterator_t
	{
		template <typename C2>
		friend auto operator == (filtered_range_iterator_t<C2> const&, filtered_range_iterator_t<C2> const&) -> bool;

		using owner_t = C;
		using source_iterator_t = typename owner_t::source_iterator_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = typename owner_t::reference;
		using const_reference   = typename owner_t::const_reference;

		using either_reference  = std::conditional_t<std::is_const<owner_t>::value, const_reference, reference>;


		filtered_range_iterator_t(C*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() -> either_reference;
		auto operator ++() -> filtered_range_iterator_t&;

	private:
		C* owner_;
		source_iterator_t pos_, end_;
	};




	//======================================================================
	// RANGE IMPLEMENTATION
	//======================================================================
	template <typename C, typename F>
	template <typename CC, typename FF>
	inline filtered_range_t<C, F>::filtered_range_t(CC&& source, source_iterator_t const& begin, source_iterator_t const& end, FF&& predicate)
		: container_(std::forward<CC>(source)), begin_(begin), end_(end), predicate_(std::forward<FF>(predicate))
	{
	}

	template <typename C, typename F>
	inline filtered_range_t<C, F>::filtered_range_t(filtered_range_t const& rhs)
		: container_(rhs.container_), begin_(rhs.begin_), end_(rhs.end_), predicate_(rhs.predicate_)
	{}

	template <typename C, typename F>
	inline auto filtered_range_t<C, F>::begin() -> either_iterator
	{
		auto i = begin_;
		while (i != end_ && !xtm::curry(predicate_)(*i))
			++i;

		return{this, i, end_};
	}

	template <typename C, typename F>
	inline auto filtered_range_t<C, F>::end() -> either_iterator
	{
		return{this, end_, end_};
	}

	template <typename C, typename F>
	inline auto filtered_range_t<C, F>::begin() const -> const_iterator
	{
		auto i = begin_;
		while (i != end_ && !predicate_(*i))
			++i;

		return{this, i, end_};
	}

	template <typename C, typename F>
	inline auto filtered_range_t<C, F>::end() const -> const_iterator
	{
		return{this, end_, end_};
	}




	//======================================================================
	// ITERATOR IMPLEMENTATION
	//======================================================================
	template <typename C>
	inline filtered_range_iterator_t<C>::filtered_range_iterator_t(owner_t* owner, source_iterator_t const& begin, source_iterator_t const& end)
		: owner_(owner), pos_(begin), end_(end)
	{
	}

	template <typename C>
	inline auto filtered_range_iterator_t<C>::operator *() -> either_reference
	{
		return *pos_;
	}

	template <typename C>
	inline auto filtered_range_iterator_t<C>::operator ++() -> filtered_range_iterator_t&
	{
		while (++pos_ != end_ && !xtm::curry(owner_->predicate())(*pos_))
			;
		return *this;
	}

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


	//======================================================================
	// FUNCTIONS
	//======================================================================
	template <typename F, typename C>
	inline auto filter(F&& predicate, C&& container) -> filtered_range_t<C, F>
	{
		return {std::forward<C>(container), container.begin(), container.end(), std::forward<F>(predicate)};
	}

	template <typename F>
	inline auto filter(F&& predicate) -> partial_filtered_range_t<F>
	{
		return {predicate};
	}
}
