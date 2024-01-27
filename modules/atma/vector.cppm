module;

#include <atma/assert.hpp>
#include <atma/config/platform.hpp>
#include <initializer_list>

export module atma.vector;

import atma.types;
import atma.aligned_allocator;
import atma.memory;

export namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T, 4>>
	struct vector
	{
	private:
		using allocator_traits = std::allocator_traits<Allocator>;

	public:
		using value_type      = T;
		using allocator_type  = typename allocator_traits::template rebind_alloc<value_type>;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference       = value_type&;
		using const_reference = value_type const&;
		using pointer         = typename allocator_traits::pointer;
		using const_pointer   = typename allocator_traits::const_pointer;
		using iterator        = T*;
		using const_iterator  = T const*;
		using buffer_type     = atma::basic_unique_memory_t<byte, Allocator>;
		
		vector() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;
		vector(vector const&);
		vector(vector&&) noexcept;
		~vector();
		
		vector(std::initializer_list<T>);
		explicit vector(size_t size);
		explicit vector(size_t size, T const&);
		template <std::forward_iterator I>
		vector(I begin, I end);
		
		auto operator = (vector const&) -> vector&;
		auto operator = (vector&&) -> vector&; // do we want this?
		auto operator [] (int) const -> T const&;
		auto operator [] (int) -> T&;

		auto get_allocator() const -> allocator_type;

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
		auto pop_back() -> void;

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
		auto imem_guard_lt(size_t capacity) -> void;
		auto imem_guard_gt(size_t capacity) -> void;
		auto imem_recapacitize(size_t) -> void;

	private:
		using internal_memory_t = atma::basic_memory_t<T, Allocator>;

		internal_memory_t imem_;
		size_t capacity_ = 0;
		size_t size_ = 0;

		template <typename Y, typename B> friend struct vector;
	};

#define IMEM_ASSERT_ITER(iter) ATMA_ASSERT(cbegin() <= iter && iter <= cend())

	template <typename T, typename A>
	inline vector<T, A>::vector(size_t size)
		: capacity_(size)
		, size_(size)
	{
		imem_.self_allocate(capacity_);

		memory_default_construct(
			xfer_dest(imem_, size));
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(size_t size, T const& d)
		: capacity_(size)
		, size_(size)
	{
		imem_.self_allocate(capacity_);

		memory_direct_construct(
			xfer_dest(imem_, size),
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
	template <std::forward_iterator I>
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
		imem_.self_allocate(capacity_);

		memory_copy_construct(
			xfer_dest(imem_),
			xfer_src(rhs.imem_),
			size_);
	}

	template <typename T, typename A>
	inline vector<T,A>::vector(vector&& rhs) noexcept
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
		memory_destruct(xfer_dest(imem_.ptr, size_));
		imem_.get_allocator().deallocate(imem_.ptr, size_);
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
		return imem_.ptr[index];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) -> T&
	{
		return imem_.ptr[index];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::get_allocator() const -> allocator_type
	{
		return this->imem_.get_allocator();
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
		return imem_.ptr;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::cend() const -> T const*
	{
		return imem_.ptr + size_;
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
	inline auto vector<T, A>::front() const -> T const&
	{
		ATMA_ASSERT(!empty());
		return imem_.ptr[0];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::back() const -> T const&
	{
		ATMA_ASSERT(!empty());
		return imem_.ptr[size_ - 1];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::front() -> T&
	{
		ATMA_ASSERT(!empty());
		return imem_.ptr[0];
	}

	template <typename T, typename A>
	inline auto vector<T, A>::back() -> T&
	{
		ATMA_ASSERT(!empty());
		return imem_.ptr[size_ - 1];
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
		return buffer_type::make_owner(imem_.detach_ptr(), c);
	}

	template <typename T, typename A>
	inline auto vector<T, A>::clear() -> void
	{
		memory_destruct(
			xfer_dest(imem_, size_));

		imem_.self_deallocate();

		imem_.ptr = nullptr;
		size_ = 0;
		capacity_ = 0;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::reserve(size_t capacity) -> void
	{
		imem_guard_lt(capacity);
	}
	
	template <typename T, typename A>
	inline auto vector<T,A>::shrink_to_fit() -> void
	{
		imem_recapacitize(size_);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size) -> void
	{
		imem_guard_lt(size);

		if (size < size_)
		{
			memory_destruct(
				xfer_dest(imem_ + size, size_ - size));
		}
		else if (size_ < size)
		{
			memory_default_construct(
				xfer_dest(imem_ + size_, size - size_));
		}

		imem_guard_gt(size);

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size, value_type const& x) -> void
	{
		imem_guard_lt(size);
		
		if (size < size_)
		{
			memory_destruct(
				xfer_dest(imem_ + size, size_ - size));
		}
		else if (size_ < size)
		{
			memory_direct_construct(
				xfer_dest(imem_ + size_, size - size_),
				x);
		}

		imem_guard_gt(size);

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::push_back(value_type const& x) -> void
	{
		imem_guard_lt(size_ + 1);

		memory_construct_at(imem_ + size_, x);

		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		imem_guard_lt(size_ + 1);

		memory_construct_at(
			imem_ + size_,
			std::forward<T>(x));

		++size_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::pop_back() -> void
	{
		ATMA_ASSERT(size_);

		memory_destruct_at(
			xfer_dest(imem_ + size_ - 1));

		--size_;

		imem_guard_gt(size_);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto vector<T, A>::emplace_back(Args&&... args) -> reference
	{
		imem_guard_lt(size_ + 1);

		memory_construct_at(
			imem_ + size_,
			std::forward<Args>(args)...);

		++size_;

		return imem_.ptr[size_ - 1];
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
		imem_guard_lt(size_ + 1);

		memory_relocate(
			xfer_dest(imem_ + offset + 1),
			xfer_src(imem_ + offset),
			(size_ - offset));

		memory_construct_at(
			imem_ + offset,
			x);

		++size_;

		return imem_.ptr + offset;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::insert(const_iterator here, value_type&& x) -> iterator
	{
		IMEM_ASSERT_ITER(here);

		auto const offset = std::distance(cbegin(), here);

		imem_guard_lt(size_ + 1);

		memory_move(
			xfer_dest(imem_ + offset + 1),
			xfer_src(imem_ + offset),
			(size_ - offset));

		memory_construct_at(
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

		size_t const offset = std::distance(cbegin(), here);
		size_t const rangesize = std::distance(start, end);
		size_t const reloc_offset = offset + rangesize;

		imem_guard_lt(size_ + rangesize);

		if (auto const mvsz = (size_ - offset))
		{
			memory_relocate(
				xfer_dest(imem_ + reloc_offset),
				xfer_src(imem_ + offset),
				mvsz);
		}

		memory_copy_construct(
			xfer_dest(imem_ + offset, rangesize),
			start, end);

		size_ += rangesize;

		return imem_ + offset;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::erase(const_iterator here) -> void
	{
		IMEM_ASSERT_ITER(here);

		auto offset = std::distance(cbegin(), here);

		memory_destruct_at(
			xfer_dest(imem_ + offset));

		memory_move(
			xfer_dest(imem_ + offset),
			xfer_src(imem_ + offset + 1),
			(size_ - offset - 1));

		--size_;

		imem_guard_gt(size_);
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
		memory_destruct(
			xfer_dest(imem_, rangesize));

		auto newcap = imem_capsize(size_ - rangesize);
		if (newcap < capacity_)
		{
			internal_memory_t tmp = std::move(imem_);

			imem_.self_allocate(newcap);

			memory_move_construct(
				xfer_dest(imem_),
				xfer_src(tmp),
				offset);

			memory_move_construct(
				xfer_dest(imem_),
				xfer_src(tmp + offset_end),
				tailsize);

			tmp.self_deallocate(capacity_);
		}
		else
		{
			memory_move_construct(
				xfer_dest(imem_ + offset),
				xfer_src(imem_ + offset_end),
				tailsize);

			memory_destruct(
				xfer_dest(imem_ + offset_end, tailsize));
		}

		size_ -= rangesize;
		capacity_ = newcap;
	}

	//
	// takes two inputs, a new minimum capacity requirement (MCR), and our current capacity,
	// and figures out what the resultant capacity should be
	// 
	// this will return capacities smaller than our current capcity if
	// the new minimum capacity required is significantly smaller than our current
	// 
	// note: our growth-factor is 1.5
	//
	template <typename T, typename A>
	inline auto vector<T, A>::imem_capsize(size_t mcr) -> size_t
	{
		// the very minimum amount of small space is the heuristic "eight"
		if (mcr <= 8)
		{
			return 8;
		}
		//
		// with a growth factor of 1.5, the moment our MCR is less than halfish
		// our current capacity, we should scale down by double-growth
		//
		//  consider a vector that grew: [8] -> [12] -> [18]
		//       now any size <= 8 we can scale down by double-growth 2.25
		//  18 / 2.25 = 8
		//   x / 2.25 === 4x / 9
		//  18 * 4 / 9 = 8
		//
		else if (mcr <= capacity_ * 4 / 9)
		{
			return mcr;
		}
		//
		// we need to expand our current capacity by 1.5
		//
		else if (capacity_ < mcr)
		{
			auto r = std::max(capacity_, size_t(8));
			while (r < mcr)
				r = 3*r / 2;

			return r;
		}
		//
		// no change required
		//
		else
		{
			return capacity_;
		}
	}

	template <typename T, typename A>
	inline auto vector<T, A>::imem_guard_lt(size_t capacity) -> void
	{
		if (auto mcr = imem_capsize(capacity); capacity_ < mcr)
		{
			imem_recapacitize(mcr);
		}
	}

	template <typename T, typename A>
	inline auto vector<T, A>::imem_guard_gt(size_t capacity) -> void
	{
		if (auto mcr = imem_capsize(capacity); mcr < capacity_)
		{
			imem_recapacitize(mcr);
		}
	}

	template <typename T, typename A>
	inline auto vector<T,A>::imem_recapacitize(size_t newcap) -> void
	{
		if (newcap < size_)
		{
			memory_destruct(xfer_dest(imem_ + newcap, size_ - newcap));
			size_ = newcap;
		}

		if (newcap != capacity_)
		{
			auto tmp = std::move(*this);
			
			if (newcap == 0)
			{
				imem_.ptr = nullptr;
			}
			else
			{
				imem_.self_allocate(newcap);

				if (!tmp.empty())
				{
					memory_move_construct(
						xfer_dest(imem_),
						xfer_src(tmp));
				}
			}

			size_ = tmp.size();
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
}
