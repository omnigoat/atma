#pragma once

#include <atma/memory.hpp>
#include <atma/unique_memory.hpp>
#include <atma/assert.hpp>

#include <initializer_list>


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
		using buffer_type     = atma::basic_unique_memory_t<byte, Allocator>;
		
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
		auto cbegin() const -> T const*;
		auto cend() const -> T const*;
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
		auto resize(size_t) -> void;
		auto resize(size_t, value_type const&) -> void;

		auto push_back(T&&) -> void;
		auto push_back(T const&) -> void;

		template <typename... Args>
		auto emplace_back(Args&&... args) -> reference;

		template <typename H> auto assign(H begin, H end) -> void;
		auto insert(const_iterator, T const&) -> iterator;
		auto insert(const_iterator, T&&) -> iterator;
		auto insert(const_iterator, std::initializer_list<T>) -> iterator;
		template <typename H> auto insert(const_iterator, H begin, H end) -> iterator;

		auto erase(const_iterator) -> void;
		auto erase(const_iterator, const_iterator) -> void;

	private:
		auto imem_capsize(size_t minsize) -> size_t;
		auto imem_recapacitize(size_t) -> void;

	private:
		using internal_memory_t = atma::basic_memory_t<T, Allocator>;

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

#define IMEM_ASSERT_ITER(iter) \
	do { \
		ATMA_ASSERT(cbegin() <= iter && iter <= cend()); \
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

		memory::range_construct(
			dest_range_t(imem_, size));
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(size_t size, T const& d)
		: capacity_(size)
		, size_(size)
	{
		imem_.allocate(capacity_);

		memory::range_construct(
			dest_range_t(imem_, size),
			d);
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
	inline vector<T, A>::vector(I begin, I end)
		: capacity_()
		, size_()
	{
		insert(this->end(), begin, end);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector const& rhs)
		: capacity_(rhs.capacity_)
		, size_(rhs.size_)
	{
		imem_.allocate(capacity_);

		memory::range_copy_construct(
			dest_range_t(imem_),
			src_range_t(rhs.imem_),
			size_);
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
		memory::range_destruct(imem_.dest_subrange(size_));
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
	inline auto vector<T, A>::cbegin() const -> T const*
	{
		return imem_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::cend() const -> T const*
	{
		return imem_ + size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() const -> T const*
	{
		return imem_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() const -> T const*
	{
		return imem_ + size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() -> T*
	{
		return imem_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() -> T*
	{
		return imem_ + size_;
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
		return imem_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() const -> T const*
	{
		return imem_;
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
		return buffer_type{unique_memory_take_ownership, imem_.detach_ptr(), c};
	}

	template <typename T, typename A>
	inline auto vector<T, A>::clear() -> void
	{
		memory::range_destruct(
			dest_range_t(imem_, size_));

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
	inline auto vector<T,A>::resize(size_t size) -> void
	{
		IMEM_GUARD_LT(size);

		if (size < size_)
		{
			memory::range_destruct(
				imem_.dest_subrange(size, size_ - size));
		}
		else if (size_ < size)
		{
			memory::range_construct(
				imem_.dest_subrange(size_, size - size_));
		}

		IMEM_GUARD_GT(size);

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size, value_type const& x) -> void
	{
		IMEM_GUARD_LT(size);
		
		if (size < size_)
		{
			memory::destruct(
				imem_.dest_range(size, size_ - size));
		}
		else if (size_ < size)
		{
			memory::range_construct(
				imem_.dest_range(size_, size - size_),
				x);
		}

		IMEM_GUARD_GT(size);

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::push_back(value_type const& x) -> void
	{
		IMEM_GUARD_LT(size_ + 1);

		memory::construct(imem_ + size_, x);

		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		IMEM_GUARD_LT(size_ + 1);

		imem_.construct(size_, std::move(x));
		++size_;
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto vector<T, A>::emplace_back(Args&&... args) -> reference
	{
		IMEM_GUARD_LT(size_ + 1);

		imem_.construct(size_, std::forward<Args>(args)...);
		++size_;

		return imem_[size_ - 1];
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T, A>::assign(H begin, H end) -> void
	{
		clear();
		new (this) vector{begin, end};
	}

	template <typename T, typename A>
	inline auto vector<T, A>::insert(const_iterator here, T const& x) -> iterator
	{
		IMEM_ASSERT_ITER(here);

		auto const offset = std::distance(cbegin(), here);
		IMEM_GUARD_LT(size_ + 1);

		memory::memmove(
			dest_range_t(imem_ + offset + 1),
			src_range_t(imem_ + offset),
			size_ - offset);

		memory::construct(
			imem_ + offset,
			x);

		++size_;

		return imem_ + offset;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::insert(const_iterator here, T&& x) -> iterator
	{
		IMEM_ASSERT_ITER(here);

		auto const offset = std::distance(cbegin(), here);

		IMEM_GUARD_LT(size_ + 1);

		memory::memmove(
			dest_range_t(imem_ + offset + 1),
			src_range_t(imem_ + offset),
			size_ - offset);

		memory::construct(
			imem_ + offset,
			std::move(x));

		++size_;

		return imem_ + offset;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::insert(const_iterator here, std::initializer_list<T> list) -> iterator
	{
		return insert(here, list.begin(), list.end());
	}

	inline size_t sub_sat(size_t x, size_t y)
	{
		size_t res = x - y;
		res &= -(res <= x);
		return res;
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T,A>::insert(const_iterator here, H start, H end) -> iterator
	{
		IMEM_ASSERT_ITER(here);

		auto const offset = std::distance(cbegin(), here);
		auto const rangesize = std::distance(start, end);
		auto const reloc_offset = offset + rangesize;

		IMEM_GUARD_LT(size_ + rangesize);

		if (auto const mvsz = size_ - offset)
		{
			if constexpr (std::is_trivial_v<value_type>)
			{
				memory::memmove(
					dest_range_t(imem_ + reloc_offset),
					src_range_t(imem_ + offset),
					mvsz);
			}
			else
			{
				memory::relocate_range(
					imem_.dest_subrange(reloc_offset, mvsz),
					imem_.src_subrange(offset, mvsz));
			}
		}

		static_assert(concepts::models_v<concepts::forward_iterator_concept, H>);
		//decltype(dest_range_t(imem_ + offset, rangesize));
		static_assert(concepts::models_v<memory_concept, decltype(dest_range_t(imem_ + offset, rangesize))>);

		memory::range_copy_construct(
			dest_range_t(imem_ + offset, rangesize),
			start);

		size_ += rangesize;

		return imem_ + offset;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::erase(const_iterator here) -> void
	{
		IMEM_ASSERT_ITER(here);

		auto offset = std::distance(cbegin(), here);

		imem_.destruct(offset, 1);
		imem_.memmove(offset, offset + 1, size_ - offset - 1);
		--size_;

		IMEM_GUARD_GT(size_);
	}

	template <typename T, typename A>
	inline auto vector<T, A>::erase(const_iterator begin, const_iterator end) -> void
	{
		ATMA_ASSERT(begin <= end);

		auto const offset = begin - this->begin();
		auto const offset_end = end - this->begin();
		auto const rangesize = size_t(end - begin);
		auto const tailsize = size_ - offset_end;

		// destruct elements in the range
		memory::range_destruct(
			dest_range_t{imem_, rangesize});

		auto newcap = imem_capsize(size_ - rangesize);
		if (newcap < capacity_)
		{
			internal_memory_t tmp = std::move(imem_);

			imem_.allocate(newcap);

			memory::range_move_construct(
				dest_range_t(imem_),
				src_range_t(tmp),
				offset);

			memory::range_move_construct(
				dest_range_t(imem_),
				src_range_t(tmp + offset_end),
				tailsize);

			tmp.deallocate(capacity_);
		}
		else
		{
			memory::range_move_construct(
				dest_range_t(imem_ + offset),
				src_range_t(imem_ + offset_end),
				tailsize);

			memory::range_destruct(
				dest_range_t(imem_ + offset_end, tailsize));
		}

		size_ -= rangesize;
		capacity_ = newcap;
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
			memory::range_destruct(imem_.dest_subrange(newcap, size_ - newcap));
			size_ = newcap;
		}

		if (newcap != capacity_)
		{
			auto tmp = imem_;

			if (newcap == 0)
			{
				imem_ = nullptr;
			}
			else
			{
				imem_.allocate(newcap);
				memory::range_move_construct(
					imem_.dest_subrange(),
					src_range_t(tmp, size_));
			}

			memory::range_destruct(tmp.dest_subrange(size_));
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
