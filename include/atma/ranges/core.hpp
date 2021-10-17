#pragma once

#include <atma/assert.hpp>

#include <functional>
#include <ranges>

import atma.types;

// range
namespace atma
{
	// sized_and_contiguous_range
	template <typename R>
	concept sized_and_contiguous_range =
		std::ranges::sized_range<R> &&
		std::ranges::contiguous_range<R>;

	// range_of_element_type
	template <typename Range, typename ElementType>
	concept range_of_element_type =
		std::ranges::range<Range> &&
		std::same_as<std::ranges::range_value_t<Range>, ElementType>;
}

// span
namespace atma
{
	constexpr inline size_t dynamic_extent = ~size_t();

	template <typename T, size_t Extent = dynamic_extent>
	struct span_t;

	// version with a known extent
	template <typename T, size_t Extent>
	struct span_t
	{
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using different_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = T const*;
		using reference = T&;
		using const_reference = T const&;
		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		template <size_t X = Extent, typename = std::enable_if_t<X == 0>>
		constexpr span_t() noexcept
		{}

		constexpr span_t(span_t const&) noexcept = default;

		constexpr span_t(pointer begin, pointer end) noexcept
			: data_(begin)
		{
			ATMA_ASSERT((end - begin) == Extent);
		}

		constexpr span_t(pointer begin, size_t size) noexcept
			: data_(begin)
		{
			ATMA_ASSERT(size == Extent);
		}

		// iterators
		constexpr auto begin() const { return data_; }
		constexpr auto end() const { return data_ + size(); }
		constexpr auto rbegin() const { return reverse_iterator(end()); }
		constexpr auto rend() const { return reverse_iterator(begin()); }

		// accessors
		constexpr auto data() const { return data_; }
		constexpr auto front() const { return (data_[0]); }
		constexpr auto back() const { return (data_[size() - 1]); }
		constexpr auto operator [] (size_type idx) const { return (data_[idx]); }

		// observers
		constexpr auto size() const { return Extent; }
		constexpr auto size_bytes() const { return Extent * sizeof(value_type); }
		[[nodiscard]] constexpr auto empty() const { return false; }

		// subviews
		constexpr auto first(size_type sz) const { return span_t<T>{data_, sz}; }
		constexpr auto last(size_type sz) const { return span_t<T>{data_ + size() - sz, sz}; }
		template <size_t Count> constexpr auto first() const { return span_t<T, Count>{data_}; }
		template <size_t Count> constexpr auto last() const { return span_t<T, Count>{data_ + size() - Count}; }

	private:
		T* data_ = nullptr;
	};


	template <typename T>
	struct span_t<T, dynamic_extent>
	{
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using different_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = T const*;
		using reference = T&;
		using const_reference = T const&;
		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		
		constexpr span_t() noexcept = default;
		constexpr span_t(span_t const&) noexcept = default;

		constexpr span_t(pointer begin, pointer end) noexcept
			: data_(begin)
			, size_(end - begin)
		{}

		constexpr span_t(pointer begin, size_t size) noexcept
			: data_(begin)
			, size_(size)
		{}

		// iterators
		constexpr auto begin() const { return data_; }
		constexpr auto end() const { return data_ + size_; }
		constexpr auto rbegin() const { return reverse_iterator(end()); }
		constexpr auto rend() const { return reverse_iterator(begin()); }

		// accessors
		constexpr auto data() const { return data_; }
		constexpr auto front() const { return (data_[0]); }
		constexpr auto back() const { return (data_[size_ - 1]); }
		constexpr auto operator [] (size_type idx) const { return (data_[idx]); }

		// observers
		constexpr auto size() const { return size_; }
		constexpr auto size_bytes() const { return size_ * sizeof(value_type); }
		constexpr auto empty() const { return size_ == 0; }

		// subviews
		constexpr auto first(size_type size) const { return span_t<T>{data_, size}; }
		constexpr auto last(size_type size) const { return span_t<T>{data_ + size_ - size, size}; }
		template <size_t Count> constexpr auto first() const { return span_t<T, Count>{data_}; }
		template <size_t Count> constexpr auto last() const { return span_t<T, Count>{data_ + size_ - Count}; }

	private:
		T* data_ = nullptr;
		size_t size_ = 0;
	};


	// deduction guides
	template <typename It, typename End>
	span_t(It, End) -> span_t<std::remove_reference_t<iter_reference_t<It>>>;

	template <typename T, size_t N>
	span_t(T(&)[N]) -> span_t<T, N>;

	template <typename T, size_t N>
	span_t(std::array<T, N>&) -> span_t<T, N>;
	
	template <typename T, size_t N>
	span_t(const std::array<T, N>&) -> span_t<const T, N>;
}

namespace atma
{
	template <typename T>
	inline auto pointer_range(T* begin, T* end) -> span_t<T>
	{
		return span_t{begin, end};
	}
}


// range_function_invoke
namespace atma
{
	namespace detail
	{
		template <typename F, typename Arg>
		decltype(auto) range_function_invoke_impl(F&& f, Arg&& arg)
		{
			return std::invoke(std::forward<F>(f), std::forward<Arg>(arg));
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...> const& tuple) {
			return std::apply(std::forward<F>(f), tuple);
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...>& tuple) {
			return std::apply(std::forward<F>(f), tuple);
		}

		template <typename F, typename... Args>
		decltype(auto) range_function_invoke_impl(F&& f, std::tuple<Args...>&& tuple) {
			return std::apply(std::forward<F>(f), std::move(tuple));
		}
	}

	template <typename F, typename Arg>
	decltype(auto) range_function_invoke(F&& f, Arg&& arg)
	{
		return detail::range_function_invoke_impl(std::forward<F>(f), std::forward<Arg>(arg));
	}
}

// ranges::for_each
namespace atma
{
	template <typename F>
	struct for_each_fn
	{
		template <typename FF>
		for_each_fn(FF&& f)
			: f_{std::forward<FF>(f)}
		{}

		template <typename R>
		void operator ()(R&& range) const
		{
			for (auto&& x : std::forward<R>(range))
				std::invoke(f_, std::forward<decltype(x)>(x));
		}

	private:
		F f_;
	};

	template <typename R, typename F>
	requires std::ranges::range<R>
	inline auto operator | (R&& range, for_each_fn<F> const& f) -> void
	{
		f(std::forward<R>(range));
	}

	template <typename F>
	inline auto for_each(F&& f)
	{
		return for_each_fn<F>{std::forward<F>(f)};
	}
}


