#pragma once

#include <atma/bind.hpp>
#include <atma/assert.hpp>
#include <atma/function.hpp>

namespace atma
{
	template <typename C, typename F> struct filtered_range_t;
	template <typename R> struct filtered_range_iterator_t;


	template <typename C, typename F>
	struct filtered_range_t
	{
		using source_container_t = std::remove_reference_t<C>;
		using source_iterator_t  = decltype(std::declval<source_container_t>().begin());

		using value_type      = typename source_container_t::value_type;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using iterator        = filtered_range_iterator_t<filtered_range_t>;
		using const_iterator  = filtered_range_iterator_t<filtered_range_t const>;
		using difference_type = typename source_container_t::difference_type;
		using size_type       = typename source_container_t::size_type;

		// if our source container is const, then even if we are not const, we can only provide const iterators.
		using either_iterator = std::conditional_t<std::is_const_v<source_container_t>, const_iterator, iterator>;


		template <typename CC, typename FF>
		filtered_range_t(CC&&, FF&&);
		filtered_range_t(filtered_range_t&&);
		filtered_range_t(filtered_range_t const&) = delete;

		auto source_container() const -> C { return container_; }
		auto predicate() const -> F const& { return predicate_; }

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
		decltype(auto) operator ()(C&& xs)
		{
			return filtered_range_t<C, F>(
				std::forward<C>(xs),
				std::forward<F>(fn_));
		}

		decltype(auto) predicate() { return fn_; }

	private:
		F fn_;
	};



	// optimized function composition for two partial filter-ranges
	template <typename F, typename G>
	struct function_composition_override<partial_filtered_range_t<F>, partial_filtered_range_t<G>>
	{
		template <typename FF, typename GG>
		static decltype(auto) compose(FF&& f, GG&& g)
		{
			auto b = atma::bind(
				[](auto f, auto g, auto x) { return f(x) && g(x); },
				f.predicate(), g.predicate(), arg1);

			return partial_filtered_range_t<decltype(b)>{b};
		}
	};



	template <typename C>
	struct filtered_range_iterator_t
	{
		using owner_t = std::remove_reference_t<C>;
		using source_iterator_t = typename owner_t::source_iterator_t;
		//using source_value_type_t = 
		constexpr static bool is_const = std::is_const_v<owner_t> || std::is_const_v<typename owner_t::value_type>;

		using iterator_category = std::forward_iterator_tag;
		using value_type        = typename owner_t::value_type;
		using difference_type   = ptrdiff_t;
		using distance_type     = ptrdiff_t;
		using pointer           = value_type*;
		using reference         = std::conditional_t<is_const, value_type const&, value_type&>;

		filtered_range_iterator_t() : owner_() {}
		filtered_range_iterator_t(owner_t*, source_iterator_t const& begin, source_iterator_t const& end);

		auto operator  *() -> reference;
		auto operator ++() -> filtered_range_iterator_t&;

	private:
		C* owner_;
		source_iterator_t pos_, end_;

		template <typename C2>
		friend auto operator == (filtered_range_iterator_t<C2> const&, filtered_range_iterator_t<C2> const&) -> bool;
	};




	//======================================================================
	// RANGE IMPLEMENTATION
	//======================================================================
	template <typename C, typename F>
	template <typename CC, typename FF>
	inline filtered_range_t<C, F>::filtered_range_t(CC&& source, FF&& predicate)
		: container_{std::forward<CC>(source)}
		, begin_{container_.begin()}
		, end_{container_.end()}
		, predicate_(std::forward<FF>(predicate))
	{
	}

	template <typename C, typename F>
	inline filtered_range_t<C, F>::filtered_range_t(filtered_range_t&& rhs)
		: container_{std::move(rhs.container_)}
		, begin_{container_.begin()}
		, end_{container_.end()}
		, predicate_{std::move(rhs.predicate_)}
	{
	}

	template <typename C, typename F>
	inline auto filtered_range_t<C, F>::begin() -> either_iterator
	{
		auto i = begin_;
		while (i != end_ && !predicate_(*i))
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
	inline auto filtered_range_iterator_t<C>::operator *() -> reference
	{
		return *pos_;
	}

	template <typename C>
	inline auto filtered_range_iterator_t<C>::operator ++() -> filtered_range_iterator_t&
	{
		do {
			++pos_;
		} while (pos_ != end_ && !call_fn(owner_->predicate(), *pos_));

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
	inline decltype(auto) filter(F&& predicate, C&& container)
	{
		return filtered_range_t<C, F>{std::forward<C>(container), std::forward<F>(predicate)};
	}

	template <typename F, typename C, typename F2>
	inline decltype(auto) filter(F&& predicate, filtered_range_t<C, F2>&& container)
	{
		auto f = atma::bind(
			[](auto f, auto g, auto x) { return f(x) && g(x); }, 
			std::forward<F>(predicate), container.predicate(), arg1);
		
		return filtered_range_t<C, decltype(f)>{std::move(container.source_container()), f};
	}

	template <typename F>
	inline decltype(auto) filter(F&& predicate)
	{
		return partial_filtered_range_t<F>{predicate};
	}
}
