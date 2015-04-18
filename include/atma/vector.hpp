#pragma once

#include <atma/unique_memory.hpp>

#include <initializer_list>
#include <allocators>


namespace atma
{
	namespace detail
	{
		inline auto quantize_memory_size(size_t size) -> size_t
		{
			auto c = size_t{8};
			while (c < size)
				c *= 2;

			return c;
		}


		template <typename A, typename T, bool E>
		struct base_internal_memory_t
		{
			auto allocate_mem(size_t capacity) -> T*;
			auto deallocate() -> void;
			auto construct_default(size_t offset, size_t count) -> void;
			auto construct_move(size_t offset, T&&) -> void;
			auto construct_copy(size_t offset, T const&) -> void;
			auto construct_copy_range(size_t offset, T const*, size_t size) -> void;
			auto destruct(size_t offset, size_t count) -> void;
			auto detach() -> T*;
			auto move(size_t dest, size_t src, size_t count) -> void;
		};

		template <typename A, typename T, bool E = std::is_empty<A>::value>
		struct internal_memory_t : base_internal_memory_t<A,T,E>
		{
			internal_memory_t()
				: allocator_{}
				, memory{}
			{}

			internal_memory_t(internal_memory_t&& rhs)
				: allocator_{std::move(rhs.allcoator_)}
				, memory{rhs.memory}
			{
				rhs.memory = nullptr;
			}

			~internal_memory_t()
			{
				allocator().deallocate(memory);
			}

			template <typename AA, typename TT>
			internal_memory_t(AA&& a, TT&& t)
				: allocator_(std::forward<AA>(a))
				, memory(std::forward<TT>(t))
			{}

			auto allocator() -> A& { return allocator_; }

			

			T* memory;

		private:
			A allocator_;
		};


		template <typename A, typename T>
		struct internal_memory_t<A, T, true>
			: base_internal_memory_t<A,T,true>
			, protected A
		{
			internal_memory_t()
				: A{}
				, memory{}
			{}

			internal_memory_t(internal_memory_t&& rhs)
				: A(std::move((A&&)rhs))
				, memory{rhs.memory}
			{
				rhs.memory = nullptr;
			}

			~internal_memory_t()
			{
				allocator().deallocate(memory, typename A::size_type{});
			}

			template <typename AA, typename TT>
			internal_memory_t(AA&& a, TT&& t)
				: A(std::forward<AA>(a)) memory_(std::forward<TT>(t))
			{}

			auto allocator() -> A& { return *this; }

			T* memory;
		};




		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A,T,E>::allocate_mem(size_t capacity) -> T*
		{
			auto* self = static_cast<internal_memory_t<A,T,E>*>(this);
			self->memory = (T*)self->allocator().allocate(sizeof(T) * capacity);
			return self->memory;
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::deallocate() -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			self->allocator().deallocate(self->memory, self->allocator().size_type{});
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::construct_default(size_t offset, size_t count) -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			for (auto i = offset; i != offset + count; ++i)
				new (self->memory + i) T{};
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::construct_move(size_t offset, T&& x) -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			new (self->memory + offset) T(std::move(x));
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::construct_copy(size_t offset, T const& x) -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			new (self->memory + offset) T{x};
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::construct_copy_range(size_t offset, T const* src, size_t count) -> void
		{	
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			for (auto i = 0; i != count; ++i)
				new (self->memory + offset + i) T(src[i]);
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::destruct(size_t offset, size_t count) -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			for (auto i = offset; i != offset + count; ++i)
				self->memory[i].~T();
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::move(size_t dest, size_t src, size_t size) -> void
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			std::memmove(self->memory + dest, self->memory + src, size * sizeof(T));
		}

		template <typename A, typename T, bool E>
		inline auto base_internal_memory_t<A, T, E>::detach() -> T*
		{
			auto* self = static_cast<internal_memory_t<A, T, E>*>(this);
			auto tmp = self->memory;
			self->memory = nullptr;
			return tmp;
		}
	}




	template <typename T, typename Allocator = std::allocator<T>>
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

		auto detach_buffer() -> unique_memory_t;

		auto reserve(size_t) -> void;
		auto recapacitize(size_t) -> void;
		auto shrink_to_fit() -> void;
		auto resize(size_t) -> void;

		auto push_back(T&&) -> void;

		auto insert(const_iterator, T const&) -> void;
		auto insert(const_iterator, T&&) -> void;
		auto insert(const_iterator, std::initializer_list<T>) -> void;
		template <typename H> auto insert(const_iterator, H begin, H end) -> void;

		auto erase(const_iterator) -> void;
		auto erase(const_iterator, const_iterator) -> void;

	private:
		auto imem_grow(size_t minsize) -> void;

	private:
		using internal_memory_t = detail::internal_memory_t<Allocator, T>;

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
		imem_.allocate_mem(capacity_);
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
		imem_.allocate_mem(capacity_);
		imem_.construct_copy_range(0, rhs.imem_.memory, size_);
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
		return imem_.memory[index];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::operator [] (int index) -> T&
	{
		return imem_.memory[index];
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
		return imem_.memory;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() const -> T const*
	{
		return imem_.memory + size_;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::begin() -> T*
	{
		return imem_.memory;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::end() -> T*
	{
		return imem_.memory + size_;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() -> T*
	{
		return imem_.memory;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::data() const -> T const*
	{
		return imem_.memory;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::empty() const -> bool
	{
		return size_ == 0;
	}

	template <typename T, typename A>
	inline auto vector<T, A>::detach_buffer() -> unique_memory_t
	{
		auto c = capacity_;
		size_ = 0;
		capacity_ = 0;
		return unique_memory_t{unique_memory_take_ownership_tag{}, imem_.detach(), c};
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
		imem_.allocate_mem(capacity);
		memcpy(imem_.memory, tmp.memory, size_ * sizeof(T));

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
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		if (size_ == capacity_)
			imem_grow(size_ + 1);

		//new (elements_ + size_++) T(std::forward<T>(x));
		imem_.construct_move(size_++, std::move(x));
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

		imem_.move(offset + 2, offset + 1, 1);
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

		imem_.move(offset + 1 + rangesize, offset + 1, size_ - offset);
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
			imem_.allocate_mem(newcap);
			memcpy(imem_.memory, tmp.memory, offset * sizeof(T));
			memcpy(imem_.memory + offset, tmp.memory + offset_end, (size_ - offset_end) * sizeof(T));
		}
		else
		{
			imem_.move(offset, offset_end, size_ - offset_end);
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
