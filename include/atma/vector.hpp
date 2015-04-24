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
	}

	template <typename T, typename Allocator = atma::aligned_allocator_t<T, 4>>
	struct vector
	{
		typedef T const* const_iterator;
		typedef T* iterator;
		typedef T& reference_type;

		vector();
		vector(size_t capacity, size_t size);
		vector(std::initializer_list<T>);
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

		auto detach_buffer() -> basic_unique_memory_t<Allocator>;
		auto attach_buffer(basic_unique_memory_t<Allocator>&&) -> void;

		auto reserve(size_t) -> void;
		auto recapacitize(size_t) -> void;
		auto shrink_to_fit() -> void;
		auto resize(size_t) -> void;

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
		using internal_memory_t = atma::memory_t<Allocator>;

		internal_memory_t imem_;
		size_t capacity_;
		size_t size_;
	};





	template <typename T, typename A>
	inline vector<T,A>::vector()
		: capacity_(), size_()
	{
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(size_t capacity, size_t size)
		: capacity_(capacity), size_(size)
	{
		imem_.alloc(capacity_);
		imem_.construct_default(0, size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(std::initializer_list<T> range)
		: capacity_(), size_()
	{
		insert(end(), range.begin(), range.end());
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector const& rhs)
		: capacity_(rhs.capacity_), size_(rhs.size_)
	{
		imem_.alloc(capacity_);
		imem_.construct_copy_range(0, rhs.imem_.ptr, size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector&& rhs)
		: imem_{std::move(rhs.imem_)}
		, capacity_(rhs.capacity_)
		, size_(rhs.size_)
	{
		rhs.capacity_ = 0;
		rhs.size_ = 0;
	}

	template <typename T, typename A>
	inline vector<T,A>::~vector()
	{
		imem_.destruct(0, size_);
		imem_.deallocate();
	}

	template <typename T, typename A>
	auto vector<T,A>::operator = (vector const& rhs) -> vector&
	{
		this->~vector();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator = (vector&& rhs) -> vector&
	{
		this->~vector();
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
	inline auto vector<T, A>::attach_buffer(basic_unique_memory_t<A>&& buf) -> void
	{
		detach_buffer();
		imem_ = buf.detach_memory();
	}

	template <typename T, typename A>
	inline auto vector<T, A>::detach_buffer() -> basic_unique_memory_t<A>
	{
		auto c = capacity_;
		size_ = 0;
		capacity_ = 0;
		return basic_unique_memory_t<A>{unique_memory_take_ownership_tag{}, imem_.detach_ptr(), c};
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
			imem_.destruct(capacity, size_ - capacity);
			size_ = capacity;
		}

		auto tmp = internal_memory_t{std::move(imem_)};
		imem_.alloc(capacity);
		imem_.memcpy(0, tmp, 0, size_);
		tmp.deallocate();

		capacity_ = capacity;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::shrink_to_fit() -> void
	{
		recapacitize(size_);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size) -> void
	{
		imem_grow(size);
		
		if (size < size_) {
			imem_.destruct(size, size_ - size);
		}
		else if (size_ < size) {
			imem_.construct_default(size_, size - size_);
		}

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::push_back(T const& x) -> void
	{
		if (size_ == capacity_)
			imem_grow(size_ + 1);

		imem_.construct_copy(size_++, x);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		if (size_ == capacity_)
			imem_grow(size_ + 1);

		imem_.construct_move(size_++, std::move(x));
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T, A>::assign(H begin, H end) -> void
	{
		imem_.deallocate();

		auto size = std::distance(begin, end);
		imem_.alloc(size);

		int idx = 0;
		for (auto i = begin; i != end; ++i)
			new (imem_.ptr + idx++) T(*i);
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
		auto const rangesize = end - begin;

		if (size_ + rangesize > capacity_)
			imem_grow(size_ + rangesize);

		imem_.memmove(offset + 1 + rangesize, offset + 1, size_ - offset);
		imem_.construct_copy_range(offset, begin, rangesize);
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
}
