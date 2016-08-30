#pragma once

#include <atma/memory.hpp>
#include <atma/unique_memory.hpp>

#include <initializer_list>
#include <allocators>


namespace atma
{
	namespace detail
	{
		template <typename I>
		using valid_iterator_t = std::enable_if<std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<I>::iterator_category>::value>;
	}

	template <typename T, typename Allocator = atma::aligned_allocator_t<T, 4>>
	struct vector
	{
		using value_type      = T;
		using allocator_type  = Allocator;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using pointer         = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer   = typename std::allocator_traits<Allocator>::const_pointer;
		using iterator        = T*;
		using const_iterator  = T const*;
		using buffer_type     = atma::basic_unique_memory_t<Allocator>;
		
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
		auto front() const -> T const&;
		auto back() const -> T const&;
		auto front() -> T&;
		auto back() -> T&;
		auto data() -> T*;
		auto data() const -> T const*;
		auto empty() const -> bool;

		auto detach_buffer() -> buffer_type;
		auto attach_buffer(buffer_type&&) -> void;
		template <typename Y, typename B> auto copy_buffer(vector<Y, B> const&) -> void;

		auto clear() -> void;
		auto reserve(size_t) -> void;
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
		auto imem_capsize(size_t minsize) -> size_t;
		auto imem_recapacitize(size_t) -> void;

	private:
		using internal_memory_t = atma::memory_t<T, Allocator>;

		internal_memory_t imem_;
		size_t capacity_;
		size_t size_;

		template <typename Y, typename B> friend struct vector;
	};




#define IMEM_GUARD_LT(capacity) \
	do { \
		if (capacity_ < capacity) \
			imem_recapacitize(imem_capsize(capacity)); \
	} while(0)

#define IMEM_GUARD_GT(capacity) \
	do { \
		if (capacity < capacity_) \
			imem_recapacitize(imem_capsize(capacity)); \
	} while(0)




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
		imem_.allocate(capacity_);
		imem_.construct(0, size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(size_t size, T const& d)
		: capacity_(size), size_(size)
	{
		imem_.allocate(capacity_);
		imem_.construct(0, size_, d);
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
		imem_.allocate(capacity_);
		imem_.construct_copy_range(0, rhs.imem_.data(), size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector&& rhs)
		: imem_(rhs.imem_)
		, capacity_(rhs.capacity_)
		, size_(rhs.size_)
	{
		rhs.imem_ = nullptr;
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
		clear();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator = (vector&& rhs) -> vector&
	{
		clear();
		new (this) vector(std::move(rhs));
		return *this;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) const -> T const&
	{
		return imem_[index];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) -> T&
	{
		return imem_[index];
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
		return imem_.data();
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() const -> T const*
	{
		return imem_.data() + size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() -> T*
	{
		return imem_.data();
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() -> T*
	{
		return imem_.data() + size_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::front() const -> T const&
	{
		ATMA_ASSERT(!empty());
		return imem_[0];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::back() const -> T const&
	{
		ATMA_ASSERT(!empty());
		return imem_[size_ - 1];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::front() -> T&
	{
		ATMA_ASSERT(!empty());
		return imem_[0];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::back() -> T&
	{
		ATMA_ASSERT(!empty());
		return imem_[size_ - 1];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() -> T*
	{
		return imem_.data();
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() const -> T const*
	{
		return imem_.data();
	}

	template <typename T, typename A>
	inline auto vector<T, A>::empty() const -> bool
	{
		return size_ == 0;
	}

#if 0
	template <typename T, typename A>
	template <typename Y, typename B>
	inline auto vector<T, A>::copy_buffer(vector<Y, B> const& rhs) -> void
	{
		clear();

		// reinterpret the data from rhs
		auto oursize = rhs.size() * sizeof(Y);
		ATMA_ASSERT(oursize % sizeof(T) == 0);
		oursize /= sizeof(T);

		imem_.allocate(oursize);
		//imem_.memcpy(0, rhs.imem_, 0, oursize);
	}
#endif

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
		imem_.destruct(0, size_);
		imem_.deallocate();
		imem_ = nullptr;
		size_ = 0;
		capacity_ = 0;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::reserve(size_t capacity) -> void
	{
		IMEM_GUARD_LT(capacity);
	}
	
	template <typename T, typename A>
	inline auto vector<T,A>::shrink_to_fit() -> void
	{
		imem_recapacitize(size_);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size, value_type const& x) -> void
	{
		IMEM_GUARD_LT(size);
		
		if (size < size_) {
			imem_.destruct(size, size_ - size);
		}
		else if (size_ < size) {
			imem_.construct(size_, size - size_, x);
		}

		IMEM_GUARD_GT(size);

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::push_back(T const& x) -> void
	{
		IMEM_GUARD_LT(size_ + 1);

		imem_.construct(size_, 1, x);
		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		IMEM_GUARD_LT(size_ + 1);

		imem_.construct(size_, 1, std::move(x));
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

		IMEM_GUARD_LT(size_ + 1);

		imem_.move(offset + 2, offset + 1, 1);
		imem_.construct(offset + 1, 1, x);
		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::insert(const_iterator where, T&& x) -> void
	{
		auto offset = where - elements_;

		IMEM_GUARD_LT(size_ + 1);

		imem_.memmove(offset + 2, offset + 1, 1);
		imem_.construct(offset + 1, 1, std::move(x));
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

		IMEM_GUARD_LT(size_ + rangesize);

		imem_.memmove(offset + 1 + rangesize, offset + 1, size_ - offset);
		imem_.construct_copy_range(offset, begin, end);
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
		auto const tailsize = size_ - offset_end;

		imem_.destruct(offset, rangesize);

		auto newcap = imem_capsize(size_ - rangesize);
		if (newcap < capacity_)
		{
			auto tmp = internal_memory_t{std::move(imem_)};
			imem_.allocate(newcap);
			imem_.construct_move_range(0, tmp, 0, offset);
			imem_.construct_move_range(offset, tmp, offset_end, tailsize);
			tmp.deallocate();
		}
		else
		{
			imem_.construct_move_range(offset, imem_, offset_end, tailsize);
			imem_.destruct(offset_end, tailsize);
		}

		size_ -= rangesize;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::imem_capsize(size_t mincap) -> size_t
	{
		if (mincap < capacity_)
			if (mincap < capacity_ / 2)
				return mincap;
			else if (mincap < capacity_ - capacity_ / 3)
				return capacity_ - capacity_ / 3;
			else
				return capacity_;
		else if (capacity_ < mincap)
			return std::max(mincap, capacity_ + capacity_ / 2);
		else
			return capacity_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::imem_recapacitize(size_t newcap) -> void
	{
		if (newcap < size_)
		{
			imem_.destruct(newcap, size_ - newcap);
			size_ = newcap;
		}

		if (newcap != capacity_)
		{
			auto tmp = imem_;

			if (newcap == 0) {
				imem_ = nullptr;
			}
			else {
				imem_.allocate(newcap);
				imem_.construct_move_range(0, tmp.data(), size_);
			}
			
			tmp.destruct(0, size_);
			tmp.deallocate();
		}

		capacity_ = newcap;
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


	#undef IMEM_GUARD_LT
	#undef IMEM_GUARD_GT
}
