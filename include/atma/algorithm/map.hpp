#pragma once

#include <atma/function_traits.hpp>
#include <atma/function.hpp>

namespace atma
{
	template <typename C, typename F> struct mapped_range_t;
	template <typename R> struct mapped_range_iterator_t;


	template <typename C, typename F>
	struct mapped_range_t
	{
		using self_t             = mapped_range_t<C, F>;
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t  = decltype(std::declval<source_container_t>().begin());

		using value_type      = result_of_t<F(typename std::remove_reference_t<C>::value_type)>;
		using reference       = value_type;
		using const_reference = value_type const;
		using iterator        = std::conditional_t<std::is_const<source_container_t>::value, mapped_range_iterator_t<self_t const>, mapped_range_iterator_t<self_t>>;
		using const_iterator  = mapped_range_iterator_t<self_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;


		template <typename CC, typename FF> mapped_range_t(CC&&, FF&&);
		mapped_range_t(mapped_range_t const&);

		auto source_container() const -> C const& { return container_; }
		auto f() const -> F const& { return fn_; }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

	private:
		C container_;
		F fn_;
		source_iterator_t begin_, end_;
	};




	template <typename F>
	struct partial_mapped_range_t
	{
		template <typename FF>
		partial_mapped_range_t(FF&& f)
			: fn_(std::forward<FF>(f))
		{
		}

		template <typename C>
		inline auto operator ()(C&& xs) -> mapped_range_t<C, F>
		{
			return mapped_range_t<C, F>(std::forward<C>(xs), std::forward<F>(fn_));
		}

	private:
		F fn_;
	};




	template <typename C>
	struct mapped_range_iterator_t
	{
		template <typename C2>
		friend auto operator == (mapped_range_iterator_t<C2> const&, mapped_range_iterator_t<C2> const&) -> bool;

		using owner_t           = C;
		using source_iterator_t = typename owner_t::source_iterator_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;

		mapped_range_iterator_t(C*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() -> value_type;
		auto operator ++() -> mapped_range_iterator_t&;

	private:
		C* owner_;
		source_iterator_t pos_, end_;
	};




	//======================================================================
	// RANGE IMPLEMENTATION
	//======================================================================
	template <typename C, typename F>
	template <typename CC, typename FF>
	inline mapped_range_t<C, F>::mapped_range_t(CC&& source, FF&& f)
		: container_(std::forward<CC>(source)), fn_(std::forward<FF>(f)), begin_(container_.begin()), end_(container_.end())
	{
		using V = typename function_traits<F>::arg_type<0>;

		static_assert(
			!std::is_reference<V>::value || std::is_const<std::remove_reference_t<V>>::value,
			"functions that could mutate values are disallowed");
	}

	template <typename C, typename F>
	inline mapped_range_t<C, F>::mapped_range_t(mapped_range_t const& rhs)
		: container_(rhs.container_), begin_(rhs.begin_), end_(rhs.end_), fn_(rhs.fn_)
	{}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::begin() -> iterator
	{
		return {this, begin_, end_};
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::end() -> iterator
	{
		return {this, end_, end_};
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::begin() const -> const_iterator
	{
		return {this, begin_, end_};
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::end() const -> const_iterator
	{
		return {this, end_, end_};
	}




	//======================================================================
	// ITERATOR IMPLEMENTATION
	//======================================================================
	template <typename C>
	inline mapped_range_iterator_t<C>::mapped_range_iterator_t(owner_t* owner, source_iterator_t const& begin, source_iterator_t const& end)
		: owner_(owner), pos_(begin), end_(end)
	{
	}

	template <typename C>
	inline auto mapped_range_iterator_t<C>::operator *() -> value_type
	{
		return owner_->f()(*pos_);
	}

	template <typename C>
	inline auto mapped_range_iterator_t<C>::operator ++() -> mapped_range_iterator_t&
	{
		++pos_;
		return *this;
	}

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



	//======================================================================
	// FUNCTIONS
	//======================================================================
	template <typename C, typename F>
	inline auto map(F&& f, C&& xs) -> mapped_range_t<C, F>
	{
		return {std::forward<C>(xs), std::forward<F>(f)};
	}

	template <typename F>
	inline auto map(F&& f) -> partial_mapped_range_t<F>
	{
		return {f};
	}

}

