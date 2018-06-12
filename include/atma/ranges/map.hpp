#pragma once

#include <atma/function_traits.hpp>
#include <atma/function.hpp>

//
//  mapping in c++ ranges
//  ------------------------
//    'mapping' is taken a range of one thing, and generating an on-the-fly range of another thing,
//    transformed by some function. 
//
//
//
namespace atma
{
	template <typename C, typename F> struct mapped_range_t;
	template <typename R> struct mapped_range_iterator_t;


	// C: either a prvalue, or a reference (mutable or const)
	template <typename C, typename F>
	struct mapped_range_t
	{
		using self_t              = mapped_range_t<C, F>;
		using source_container_t  = std::remove_reference_t<C>;
		using storage_container_t = storage_type_t<C>;
		using source_iterator_t   = decltype(std::declval<storage_container_t>().begin());
		using invoke_result_t     = std::invoke_result_t<F, typename std::remove_reference_t<C>::value_type>;

		// the value-type of the what F returns. we must remove references
		using value_type      = std::remove_reference_t<invoke_result_t>;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using iterator        = mapped_range_iterator_t<transfer_const_t<source_container_t, self_t>>;
		using const_iterator  = mapped_range_iterator_t<self_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;


		mapped_range_t(mapped_range_t const&) = default;
		mapped_range_t(mapped_range_t&&) = default;

		template <typename CC, typename FF>
		mapped_range_t(CC&&, FF&&);

		auto source_container() const -> C const& { return container_; }
		auto f() const -> F const& { return fn_; }

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto begin() -> iterator;
		auto end() -> iterator;

	private:
		storage_container_t container_;
		F fn_;
		source_iterator_t okay_;
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
		using source_iterator_t = typename std::remove_reference_t<owner_t>::source_iterator_t;
		using invoke_result_t   = typename owner_t::invoke_result_t;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = typename owner_t::reference;

		mapped_range_iterator_t(C*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() const -> invoke_result_t;
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
		: container_(std::forward<CC>(source))
		, fn_(std::forward<FF>(f))
		, okay_{container_.begin()}
	{}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::begin() -> iterator
	{
		return iterator(this, container_.begin(), container_.end());
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::end() -> iterator
	{
		return iterator(this, container_.end(), container_.end());
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::begin() const -> const_iterator
	{
		return const_iterator{this, container_.begin(), container_.end()};
	}

	template <typename C, typename F>
	inline auto mapped_range_t<C, F>::end() const -> const_iterator
	{
		return const_iterator{this, container_.end(), container_.end()};
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
	inline auto mapped_range_iterator_t<C>::operator *() const -> invoke_result_t
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
	inline auto map(F&& f, C&& xs)
	{
		return mapped_range_t<decltype(xs), F>{std::forward<C>(xs), std::forward<F>(f)};
	}

	template <typename F>
	inline auto map(F&& f) -> partial_mapped_range_t<F>
	{
		return {f};
	}

	template <typename R, typename C>
	inline auto map(R C::*member)
	{
		auto f = [=](auto&& x) { return x.*member; };
		return partial_mapped_range_t<decltype(f)>{f};
	}

}

