#pragma once

#include <initializer_list>
#include <allocators>

namespace atma
{
	namespace detail
	{
		inline auto quantize_memory(size_t size) -> size_t
		{
			auto c = size_t{8};
			while (c < minsize)
				c *= 2;

			return c;
		}




		template <typename A, typename T, bool = std::is_empty<A>::value>
		struct internal_memory_t
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

			template <typename AA, typename TT>
			internal_memory_t(AA&& a, TT&& t)
				: allocator_(std::forward<AA>(a))
				, memory(std::forward<TT>(t))
			{}

			auto allocator() -> A& { return allocator_; }

			auto allocate(size_t capacity) -> T*;
			auto deallocate() -> void;
			auto construct_default(size_t offset, size_t count) -> void;
			auto construct_copy(size_t offset, T const*, size_t size) -> void;
			auto destruct(size_t offset, size_t count) -> void;

			T* memory;

		private:
			A allocator_;
		};


		template <typename A, typename T>
		struct internal_memory_t<A, T, true>
			: A
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

			template <typename AA, typename TT>
			internal_memory_t(AA&& a, TT&& t)
				: A(std::forward<AA>(a)) memory_(std::forward<TT>(t))
			{}

			auto allocator() -> A& { return *this; }

			auto allocate(size_t capacity) -> T*;
			auto deallocate() -> void;
			auto construct_default(size_t offset, size_t count) -> void;
			auto construct_copy(size_t offset, T const*, size_t size) -> void;
			auto destruct(size_t, size_t) -> void;

			T* memory;
		};




		template <typename A, typename T, bool E>
		inline auto internal_memory_t<A,T,E>::allocate(size_t capacity) -> T*
		{
			memory = (T*)allocator().allocate(sizeof(T) * capacity);
			return memory;
		}

		template <typename A, typename T, bool E>
		inline auto internal_memory_t<A, T, E>::deallocate() -> void
		{
			allocator().deallocate(memory, allocator().size_type{});
		}

		template <typename A, typename T, bool E>
		inline auto internal_memory_t<A, T, E>::construct_default(size_t offset, size_t count) -> void
		{
			for (auto i = offset; i != offset + count; ++i)
				new (memory + i) T{};
		}

		template <typename A, typename T, bool E>
		inline auto internal_memory_t<A, T, E>::construct_copy(size_t offset, T const* src, size_t size) -> void
		{
			for (auto i = 0; i != count; ++i)
				new (memory + offset + i) T{src + i};
		}

		template <typename A, typename T, bool E>
		inline auto internal_memory_t<A, T, E>::destruct(size_t offset, size_t count) -> void
		{
			for (auto i = offset; i != offset + count; ++i)
				memory[i].~T();
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

		auto reserve(size_t) -> void;
		auto recapacitize(size_t) -> void;
		auto shrink_to_fit() -> void;
		auto resize(size_t) -> void;

		auto push_back(T&&) -> void;
		auto insert(const_iterator, T&&) -> void;
		template <typename H> auto insert(const_iterator, H begin, H end) -> void;
		auto erase(const_iterator) -> void;

	private:
		auto imem_allocate(size_t) -> T*;
		auto imem_deallocate(T*) -> void;
		auto elements_initialize_default(T*, size_t where, size_t len) -> void;
		auto elements_initialize_value(T*, size_t where, size_t len, T const&) -> void;
		auto elements_deinitialize(T*, size_t where, size_t len) -> void;

		auto elements_grow(size_t minsize) -> void;

	private:
		detail::internal_memory_t<Allocator, T> imem_;
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
		imem_.allocate(capacity_);
		imem_.initialize_default(0, size_);
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
		imem_.allocate(capacity_);
		imem_.construct_copy(0, rhs.imem_.memory, size_);
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
	inline auto vector<T,A>::reserve(size_t capacity) -> void
	{
		auto newcap = std::max(capacity, 8);
		while (newcap < capacity_)
			newcap *= 2;

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

		//auto tmp = std::move(imem_);
		imem_.allocate(capacity);
		memcpy(imem_.memory(), tmp.memory(), size_);
		tmp.
		elements_ = imem_allocate(capacity);
		memcpy(elements_, tmp, size_);
		imem_deallocate(tmp);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::shrink_to_fit() -> void
	{
		recapacitize(size_);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::resize(size_t size) -> void
	{
		if (size < size_) {
			imem_.destruct(size, size_ - size);
		}
		
		if (capacity_ < size) {
			imem_grow(size);
		}

		if (size_ < size) {
			imem_.construct_default(size_, size - size_);
		}

		size_ = size;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::push_back(T&& x) -> void
	{
		if (size_ == capacity_)
			elements_grow(size_ + 1);

		new (elements_ + size_++) T(std::forward<T>(x));
	}

	template <typename T, typename A>
	inline auto vector<T,A>::insert(const_iterator where, T&& x) -> void
	{
		auto offset = where - elements_;

		if (size_ == capacity_)
			elements_grow(size_ + 1);

		memmove(elements_ + offset + 2, elements_ + offset + 1, size_ - offset);
		new (elements_ + offset + 1) T(x);
		++size_;
	}

	template <typename T, typename A>
	template <typename H>
	inline auto vector<T,A>::insert(const_iterator where, H begin, H end) -> void
	{
		auto const offset = where - this->begin();
		auto const rangesize = end - begin;

		if (size_ == capacity_)
			elements_grow(size_ + rangesize);

		memmove(imem_.memory + offset + 1 + rangesize, imem_.memory + offset + 1, size_ - offset);
		for (auto i = begin; i != end; ++i)
			new (imem_.memory + offset + (i - begin)) T(*i);
		size_ += rangesize;
	}

	template <typename T, typename A>
	inline auto vector<T,A>::erase(const_iterator where) -> void
	{
		ATMA_ASSERT("things");
		auto offset = where - elements_;

		if (size_ - 1 < capacity_ / 2) {
			recapacitize(capacity_ / 2);
		}

		elements_deinitialize(elements_, offset, 1);
	}


	template <typename T, typename A>
	inline auto vector<T,A>::imem_allocate(size_t capacity) -> T*
	{
		return (T*)imem_.allocator().allocate(sizeof(T) * capacity);  // (T*)new char [sizeof(T) * capacity];
	}

	template <typename T, typename A>
	inline auto vector<T,A>::imem_deallocate(T* elements) -> void
	{
		delete[] reinterpret_cast<char*>(elements);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::elements_initialize_default(T* elements, size_t where, size_t len) -> void
	{
		for (auto i = where; i != where + len; ++i)
			new (elements + i) T();
	}

	template <typename T, typename A>
	inline auto vector<T,A>::elements_initialize_value(T* elements, size_t where, size_t len, T const& x) -> void
	{
		for (auto i = where; i != where + len; ++i)
			new (elements + i) T(x);
	}

	template <typename T, typename A>
	inline auto vector<T,A>::elements_deinitialize(T* elements, size_t where, size_t len) -> void
	{
		for (auto i = where; i != where + len; ++i)
			elements[i].~T();
	}

	template <typename T, typename A>
	inline auto vector<T,A>::elements_grow(size_t minsize) -> void
	{
		auto c = std::max(minsize, size_t{8});
		auto c = size_t{8};
		while (c < minsize)
			c *= 2;
		
		auto newcap = std::max(capacity_, size_t(8));
		while (newcap < minsize)
			newcap *= 2;

		recapacitize(newcap);
	}
}
