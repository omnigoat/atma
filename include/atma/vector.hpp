#pragma once

namespace atma
{
	template <typename T>
	struct vector
	{
		typedef T const* const_iterator;
		typedef T* iterator;
		typedef T& reference_type;

		vector();
		vector(size_t capacity, size_t size);
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
		auto insert(const_iterator, T&&);
		template <typename H> auto insert(const_iterator, H begin, H end) -> void;
		auto erase(const_iterator) -> void;

	private:
		auto elements_allocate(size_t) -> T*;
		auto elements_deallocate(T*) -> void;
		auto elements_initialize_default(T*, uint where, uint len) -> void;
		auto elements_initialize_value(T*, uint where, uint len, T const&) -> void;
		auto elements_deinitialize(T*, uint where, uint len) -> void;

		auto elements_grow(size_t minsize) -> void;

	private:
		// allocator?

		T* elements_;
		size_t capacity_;
		size_t size_;
	};





	template <typename T>
	vector<T>::vector()
	: elements_(), capacity_(), size_()
	{
	}

	template <typename T>
	vector<T>::vector(size_t capacity, size_t size)
	: elements_(), capacity_(capacity), size_(size)
	{
		elements_ = elements_allocate(capacity_);
		elements_initialize_default(elements_, 0, size_);
	}

	template <typename T>
	vector<T>::vector(vector const& rhs)
	: elements_(), capacity_(rhs.capacity_), size_(rhs.size_)
	{
		elements_ = elements_allocate(capacity_);

		for (auto i = 0; i != size_; ++i)
			new (elements_ + i) T(rhs.elements_[i]);
	}

	template <typename T>
	vector<T>::vector(vector&& rhs)
	: elements_(rhs.elements_), capacity_(rhs.capacity_), size_(rhs.size_)
	{
		rhs.elements_ = nullptr;
		rhs.capacity_ = 0;
		rhs.size_ = 0;
	}

	template <typename T>
	vector<T>::~vector()
	{
		elements_deinitialize(elements_, 0, size_);
		elements_deallocate(elements_);
	}

	template <typename T>
	vector<T>::operator = (vector const& rhs) -> T&
	{
		this->~vector();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T>
	vector<T>::operator = (vector&& rhs) -> T&
	{
		this->~vector();
		new (this) vector(rhs);
		return *this;
	}

	template <typename T>
	vector<T>::operator [] (int index) const -> T const&
	{
		return elements_[index];
	}

	template <typename T>
	vector<T>::operator [] (int index) -> T&
	{
		return elements_[index];
	}

	template <typename T>
	auto vector<T>::capacity() const -> size_t
	{
		capacity_;
	}

	template <typename T>
	auto vector<T>::size() const -> size_t
	{
		return size_;
	}

	template <typename T>
	auto vector<T>::begin() const -> T const*
	{
		return elements_;
	}

	template <typename T>
	auto vector<T>::end() const -> T const*
	{
		return elements_ + size_;
	}

	template <typename T>
	auto vector<T>::begin() -> T*
	{
		return elements_;
	}

	template <typename T>
	auto vector<T>::end() -> T*
	{
		return elements_ + size_;
	}

	template <typename T>
	auto vector<T>::reserve(size_t capacity) -> void
	{
		auto newcap = std::max(capacity, 8);
		while (newcap < capacity_)
			newcap *= 2;

		recapacitize(newcap);
	}
	
	template <typename T>
	auto vector<T>::recapacitize(size_t capacity) -> void
	{
		if (capacity == capacity_)
			return;

		if (capacity < size_) {
			elements_deinitialize(capacity, size_ - capacity);
			size_ = capacity;
		}

		auto tmp = elements_;
		elements_ = elements_allocate(capacity);
		memcpy(elements_, tmp, size_);
		elements_deallocate(tmp);
	}

	template <typename T>
	auto vector<T>::shrink_to_fit() -> void
	{
		recapacitize(size_);
	}

	template <typename T>
	auto vector<T>::resize(size_t size) -> void
	{
		if (size < size_) {
			elements_deinitialize(elements_, size, size_ - size);
		}
		
		if (capacity_ < size) {
			elements_grow(size);
		}

		if (size_ < size) {
			elements_initialize_default(elements_, size_, size - size_);
		}

		size_ = size;
	}

	template <typename T>
	auto vector<T>::push_back(T&& x) -> void
	{
		if (size_ == capacity_)
			elements_grow(size_ + 1);

		new (elements_ + size_++) T(std::forward<T>(x));
	}

	template <typename T>
	auto vector<T>::insert(const_iterator where, T&& x) -> void
	{
		auto offset = where - elements_;

		if (size_ == capacity_)
			elements_grow(size_ + 1);

		memmove(elements_ + offset + 2, elements_ + offset + 1, size_ - offset);
		new (elements_ + offset + 1) T(x);
		++size_;
	}

	template <typename H>
	template <typename T>
	auto vector<T>::insert(const_iterator where, H begin, H end) -> void
	{
		auto offset = where - elements_;
		auto rangesize = end - begin;

		if (size_ == capacity_)
			elements_grow(size_ + rangesize);

		memmove(elements_ + offset + 1 + rangesize, elements_ + offset + 1, size_ - offset);
		new (elements_ + offset + 1) T(x);
		++size_;
	}

	template <typename T>
	auto vector<T>::erase(const_iterator where) -> void
	{
		ATMA_ASSERT("things");
		auto offset = where - elements_;

		if (size_ - 1 < capacity_ / 2) {
			recapacitize(capacity_ / 2);
		}

		elements_deinitialize(elements_, offset, 1);
	}



	template <typename T>
	auto vector<T>::elements_allocate(size_t capacity) -> T*
	{
		return (T*)new char [sizeof(T) * capacity];
	}

	template <typename T>
	auto vector<T>::elements_deallocate(T* elements, uint where, uint len) -> void
	{
		delete[] reinterpret_cast<char*>(elements);
	}

	template <typename T>
	auto vector<T>::elements_initialize_default(T* elements, uint where, uint len) -> void
	{
		for (auto i = where; i != where + len; ++i)
			new (elements + i) T();
	}

	template <typename T>
	auto vector<T>::elements_initialize_value(T* elements, uint where, uint len, T const& x) -> void
	{
		for (auto i = where; i != where + len; ++i)
			new (elements + i) T(x);
	}

	template <typename T>
	auto vector<T>::elements_deinitialize(T* elements, uint where, uint len) -> void
	{
		for (auto i = where; i != where + len; ++i)
			elements[i].~T();
	}

	template <typename T>
	auto vector<T>::elements_grow(size_t minsize) -> void
	{
		if (minsize < capacity_)
			return;
		
		auto newcap = std::max(capacity_, 8);
		while (newcap < minsize)
			newcap *= 2;

		recapacitize(newcap);
	}
}
