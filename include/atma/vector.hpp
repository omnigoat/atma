#pragma once

#include <atma/memory.hpp>
#include <atma/unique_memory.hpp>

#include <initializer_list>
#include <allocators>


namespace atma
{
	namespace detail
	{
		inline auto quantize_memory_size(size_t s) -> size_t
		{
			size_t r = 8;
			while (r < s)
				r *= 2;
			return r;
		}

		template <typename I>
		using valid_iterator_t = std::enable_if<std::is_same<typename std::iterator_traits<I>::iterator_category, std::forward_iterator_tag>::value>;
	}

	template <typename T, typename Allocator = atma::aligned_allocator_t<T, 4>>
	struct vector
	{
		using value_type     = T;
		using const_iterator = T const*;
		using iterator       = T*;
		using reference_type = T&;
		using buffer_type    = atma::basic_unique_memory_t<Allocator>;
		
		vector();
		explicit vector(size_t size);
		explicit vector(size_t size, T const&);
		vector(std::initializer_list<T>);
		template <typename I, typename = detail::valid_iterator_t<I>>
		vector(I begin, I end);
		vector(vector const&);
		vector(vector&&);
		~vector();

		auto operator = (vector const&) -> vector&;
		auto operator = (vector&&) -> vector&; // do we want this?
		auto operator [] (int) const -> T const&;
		auto operator [] (int) -> T&;

		auto size() const -> size_t;
		auto capacity() const -> size_t;
		auto begin() const -> T const*;
		auto end() const -> T const*;
		auto begin() -> T*;
		auto end() -> T*;
		auto data() -> T*;
		auto data() const -> T const*;
		auto empty() const -> bool;

		auto detach_buffer() -> buffer_type;
		auto attach_buffer(buffer_type&&) -> void;
		template <typename Y, typename B> auto copy_buffer(vector<Y, B> const&) -> void;

		auto clear() -> void;
		auto reserve(size_t) -> void;
		auto recapacitize(size_t) -> void;
		auto shrink_to_fit() -> void;
		auto resize(size_t, value_type const& = value_type()) -> void;

		auto push_back(T&&) -> void;
		auto push_back(T const&) -> void;

		template <typename H> auto assign(H begin, H end) -> void;
		auto insert(const_iterator, T const&) -> void;
		auto insert(const_iterator, T&&) -> void;
		auto insert(const_iterator, std::initializer_list<T>) -> void;
		template <typename H> auto insert(const_iterator, H begin, H end) -> void;

		auto erase(const_iterator) -> void;
		auto erase(const_iterator, const_iterator) -> void;

	private:
		auto imem_grow(size_t minsize) -> void;

	private:
		using internal_memory_t = atma::memory_t<T, Allocator>;

		internal_memory_t imem_;
		size_t capacity_;
		size_t size_;

		template <typename Y, typename B> friend struct vector;
	};





	template <typename T, typename A>
	inline vector<T,A>::vector()
		: capacity_()
		, size_()
	{
	}

	template <typename T, typename A>
	inline vector<T, A>::vector(size_t size)
		: capacity_(size)
		, size_(size)
	{
		memory_allocate(imem_, capacity_);
		memory_construct_default(imem_, 0, size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(size_t size, T const& d)
		: capacity_(size), size_(size)
	{
		memory_allocate(imem_, capacity_);
		memory_construct_copy(imem_, 0, size_, d);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(std::initializer_list<T> range)
		: capacity_()
		, size_()
	{
		insert(end(), range.begin(), range.end());
	}

	template <typename T, typename A>
	template <typename I, typename>
	inline vector<T, A>::vector(I begin, I endd)
		: capacity_()
		, size_()
	{
		insert(end(), begin, endd);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector const& rhs)
		: capacity_(rhs.capacity_)
		, size_(rhs.size_)
	{
		memory_allocate(imem_, capacity_);
		memory_construct_copy(imem_, 0, size_, rhs.imem_.ptr);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector&& rhs)
		: imem_(rhs.imem_)
		, capacity_(rhs.capacity_)
		, size_(rhs.size_)
	{
		rhs.imem_.ptr = nullptr;
		rhs.capacity_ = 0;
		rhs.size_ = 0;
	}

	template <typename T, typename A>
	inline vector<T,A>::~vector()
	{
		memory_destruct(imem_, 0, size_);
		memory_deallocate(imem_);
	}

	template <typename T, typename A>
	auto vector<T,A>::operator = (vector const& rhs) -> vector&
	{
		clear();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator = (vector&& rhs) -> vector&
	{
		clear();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) const -> T const&
	{
		return imem_.ptr[index];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) -> T&
	{
		return imem_.ptr[index];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::capacity() const -> size_t
	{
		return capacity_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::size() const -> size_t
	{
		return size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() const -> T const*
	{
		return imem_.ptr;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() const -> T const*
	{
		return imem_.ptr + size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() -> T*
	{
		return imem_.ptr;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() -> T*
	{
		return imem_.ptr + size_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() -> T*
	{
		return imem_.ptr;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() const -> T const*
	{
		return imem_.ptr;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::empty() const -> bool
	{
		return size_ == 0;
	}

	template <typename T, typename A>
	template <typename Y, typename B>
	inline auto vector<T, A>::copy_buffer(vector<Y, B> const& rhs) -> void
	{
		clear();

		// reinterpret the data from rhs
		auto oursize = rhs.size() * sizeof(Y);
		ATMA_ASSERT(oursize % sizeof(T) == 0);
		oursize /= sizeof(T);

		memory_allocate(imem_, oursize);
		memory_memcpy(imem_, 0, rhs.imem_, 0, oursize);
	}

	template <typename T, typename A>
	inline auto vector<T, A>::attach_buffer(buffer_type&& buf) -> void
	{
		detach_buffer();
		imem_ = buf.detach_memory();
	}

	template <typename T, typename A>
	inline auto vector<T, A>::detach_buffer() -> buffer_type
	{
		auto c = capacity_;
		size_ = 0;
		capacity_ = 0;
		return basic_unique_memory_t<A>{unique_memory_take_ownership_tag{}, imem_.detach_ptr(), c};
	}

	template <typename T, typename A>
	inline auto vector<T, A>::clear() -> void
	{
		memory_destruct(imem_, 0, size_);
		memory_deallocate(imem_);
		imem_.ptr = nullptr;
		size_ = 0;
		capacity_ = 0;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::reserve(size_t capacity) -> void
	{
		auto newcap = detail::quantize_memory_size(capacity);
		recapacitize(newcap);
	}
	
	template <typename T, typename A>
	inline auto vector<T,A>::recapacitize(size_t capacity) -> void
	{
		if (capacity == capacity_)
			return;

		if (capacity < size_) {
			memory_destruct(imem_, capacity, size_ - capacity);
			size_ = capacity;
		}

		if (capacity > 0)
		{
			auto tmp = imem_;
			memory_allocate(imem_, capacity);
			memory_memcpy(imem_, 0, tmp, 0, size_);
			memory_deallocate(tmp);
		}

		capacity_ = capacity;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::shrink_to_fit() -> void
	{
		recapacitize(size_);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size, value_type const& x) -> void
	{
		imem_grow(size);
		
		if (size < size_) {
			memory_destruct(imem_, size, size_ - size);
		}
		else if (size_ < size) {
			memory_construct_copy(imem_, size_, size - size_, x);
		}

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::push_back(T const& x) -> void
	{
		if (size_ == capacity_)
			imem_grow(size_ + 1);

		memory_construct_copy(imem_, size_, 1, x);
		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		if (size_ == capacity_)
			imem_grow(size_ + 1);

		memory_construct_move(imem_, size_, std::move(x));
		++size_;
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T, A>::assign(H begin, H end) -> void
	{
		clear();
		new (this) vector(begin, end);
	}

	template <typename T, typename A>
	inline auto vector<T, A>::insert(const_iterator where, T const& x) -> void
	{
		auto offset = where - elements_;

		if (size_ == capacity_)
			imem_grow(size_ + 1);

		imem_.move(offset + 2, offset + 1, 1);
		imem_.construct_copy(offset + 1, x, 1);
		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::insert(const_iterator where, T&& x) -> void
	{
		auto offset = where - elements_;

		if (size_ == capacity_)
			imem_grow(size_ + 1);

		imem_.memmove(offset + 2, offset + 1, 1);
		imem_.construct_move(offset + 1, x, 1);
		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::insert(const_iterator where, std::initializer_list<T> list) -> void
	{
		insert(where, list.begin(), list.end());
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T,A>::insert(const_iterator where, H begin, H end) -> void
	{
		auto const offset = where - this->begin();
		auto const rangesize = std::distance(begin, end);

		if (size_ + rangesize > capacity_)
			imem_grow(size_ + rangesize);

		memory_memmove(imem_, offset + 1 + rangesize, offset + 1, size_ - offset);
		for (auto i = offset; i != rangesize; ++i)
			new (imem_.ptr + i) T(*begin++);
		//memory_construct_copy(offset, begin, rangesize);
		size_ += rangesize;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::erase(const_iterator where) -> void
	{
		ATMA_ASSERT("things");
		auto offset = where - begin();

		imem_.destruct(offset, 1);
		imem_grow(size_ - 1);
	}

	template <typename T, typename A>
	inline auto vector<T, A>::erase(const_iterator begin, const_iterator end) -> void
	{
		auto const offset = begin - this->begin();
		auto const offset_end = end - this->begin();
		auto const rangesize = end - begin;

		imem_.destruct(offset, rangesize);

		auto newcap = detail::quantize_memory_size(size_ - rangesize);
		if (newcap != capacity_)
		{
			auto tmp = internal_memory_t{std::move(imem_)};
			imem_.alloc(newcap);
			imem_.memcpy(0, tmp, 0, offset);
			imem_.memcpy(offset, tmp, offset_end, size_ - offset_end);
			tmp.deallocate();
		}
		else
		{
			imem_.memmove(offset, offset_end, size_ - offset_end);
		}

		size_ -= rangesize;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::imem_grow(size_t minsize) -> void
	{
		auto newcap = detail::quantize_memory_size(minsize);
		recapacitize(newcap);
	}

	template <typename T, typename A>
	inline auto operator == (vector<T,A> const& lhs, vector<T, A> const& rhs) -> bool
	{
		return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}

	template <typename T, typename A>
	inline auto operator != (vector<T, A> const& lhs, vector<T, A> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}
}
