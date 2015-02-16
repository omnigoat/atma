#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

namespace atma
{
	namespace detail {
		template <typename T> struct sz { static size_t const value = sizeof(T); };
		template <> struct sz<void> { static size_t const value = 1; };
	}


	template <typename T = void>
	struct typed_unique_memory_t
	{
		explicit typed_unique_memory_t();
		explicit typed_unique_memory_t(size_t size);
		typed_unique_memory_t(size_t size, T* data);
		typed_unique_memory_t(uint alignment, size_t size);
		typed_unique_memory_t(uint alignment, size_t size, T* data);
		typed_unique_memory_t(typed_unique_memory_t&&);
		~typed_unique_memory_t();
		typed_unique_memory_t(typed_unique_memory_t const&) = delete;

		auto operator = (typed_unique_memory_t&&) -> typed_unique_memory_t&;
		auto operator = (typed_unique_memory_t const&) -> typed_unique_memory_t& = delete;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> T const*;
		auto end() const -> T const*;
		operator bool() const;

		auto begin() -> T*;
		auto end() -> T*;
		auto swap(typed_unique_memory_t&) -> void;

	private:
		T* data_;
		T* end_;
	};

	using unique_memory_t = typed_unique_memory_t<void>;



	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t()
		: data_()
		, end_()
	{
		
	}

	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t(size_t size)
		: typed_unique_memory_t(4, size * detail::sz<T>::value)
	{
	}

	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t(size_t size, T* data)
		: typed_unique_memory_t(4, size * detail::sz<T>::value, data)
	{
	}

	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t(uint alignment, size_t size)
		: data_(reinterpret_cast<T*>(platform::allocate_aligned_memory(alignment, size * detail::sz<T>::value)))
		, end_(reinterpret_cast<T*>(reinterpret_cast<char*>(data_) + size * detail::sz<T>::value))
	{
	}

	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t(uint alignment, size_t size, T* data)
		: data_(reinterpret_cast<T*>(platform::allocate_aligned_memory(alignment, size * detail::sz<T>::value)))
		, end_(reinterpret_cast<T*>(reinterpret_cast<char*>(data_)+ size * detail::sz<T>::value))
	{
		memcpy(data_, data, size);
	}

	template <typename T>
	inline typed_unique_memory_t<T>::typed_unique_memory_t(typed_unique_memory_t&& rhs)
		: data_(rhs.data_)
		, end_(rhs.end_)
	{
		rhs.data_ = nullptr;
		rhs.end_ = nullptr;
	}

	template <typename T>
	inline typed_unique_memory_t<T>::~typed_unique_memory_t()
	{
		platform::deallocate_aligned_memory(data_);
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::operator = (typed_unique_memory_t&& rhs) -> typed_unique_memory_t&
	{
		this->~typed_unique_memory_t();
		data_ = rhs.data_;
		end_ = rhs.end_;
		rhs.data_ = nullptr;
		rhs.end_ = nullptr;
		return *this;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::empty() const -> bool
	{
		return size() == 0;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::size() const -> size_t
	{
		return reinterpret_cast<char const*>(end_) - reinterpret_cast<char const*>(data_);
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::begin() -> T*
	{
		return data_;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::end() -> T*
	{
		return end_;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::begin() const -> T const*
	{
		return data_;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::end() const -> T const*
	{
		return end_;
	}

	template <typename T>
	inline auto typed_unique_memory_t<T>::swap(typed_unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}

	template <typename T>
	inline typed_unique_memory_t<T>::operator bool() const
	{
		return data_ != nullptr;
	}
}



