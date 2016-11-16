#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>
#include <atma/memory.hpp>
#include <atma/aligned_allocator.hpp>


namespace atma
{
	struct unique_memory_take_ownership_tag {};

	template <typename Alloc>
	struct basic_unique_memory_t
	{
		using backing_t = atma::memory_t<byte, Alloc>;

		basic_unique_memory_t();
		explicit basic_unique_memory_t(size_t size);
		basic_unique_memory_t(void const* data, size_t size);
		basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size);
		basic_unique_memory_t(basic_unique_memory_t const&) = delete;
		template <typename U> basic_unique_memory_t(basic_unique_memory_t<U>&&);
		~basic_unique_memory_t();

		auto operator = (basic_unique_memory_t const&) -> basic_unique_memory_t& = delete;
		template <typename U> auto operator = (basic_unique_memory_t<U>&&) -> basic_unique_memory_t&;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> byte const*;
		auto end() const -> byte const*;
		auto begin() -> byte*;
		auto end() -> byte*;

		auto reset(void* mem, size_t size) -> void;
		auto reset(size_t) -> void;

		auto swap(basic_unique_memory_t&) -> void;

		auto detach_memory() -> backing_t;

	private:
		backing_t memory_;
		size_t size_;

		template <typename U> friend struct basic_unique_memory_t;
	};



	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t()
		: size_()
	{
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(size_t size)
		: size_(size)
	{
		if (size)
			memory_.allocate(size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(void const* data, size_t size)
		: size_(size)
	{
		if (size) {
			memory_.allocate(size);
			memory_.memcpy(0, data, size);
		}
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size)
		: memory_{data}
		, size_(size)
	{
	}

	template <typename A>
	template <typename U>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(basic_unique_memory_t<U>&& rhs)
		: memory_{rhs.memory_}
		, size_{rhs.size_}
	{
		rhs.memory_ = nullptr;
		rhs.size_ = 0;
	}

	template <typename A>
	inline basic_unique_memory_t<A>::~basic_unique_memory_t()
	{
		memory_.deallocate();
	}

	template <typename A>
	template <typename U>
	inline auto basic_unique_memory_t<A>::operator = (basic_unique_memory_t<U>&& rhs) -> basic_unique_memory_t&
	{
		this->~basic_unique_memory_t();
		memory_ = rhs.memory_;
		size_ = rhs.size_;
		rhs.memory_ = nullptr;
		return *this;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::empty() const -> bool
	{
		return size() == 0;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::size() const -> size_t
	{
		return size_;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::begin() const -> byte const*
	{
		return memory_.data();
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::end() const -> byte const*
	{
		return memory_.data() + size_;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::begin() -> byte*
	{
		return memory_.data();
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::end() -> byte*
	{
		return memory_.data() + size_;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::reset(void* mem, size_t size) -> void
	{
		memory_.deallocate();
		memory_.allocate(size);
		memory_.memcpy(0, mem, size);
		size_ = size;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::reset(size_t size) -> void
	{
		memory_.deallocate();
		memory_.allocate(size);
		size_ = size;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::swap(basic_unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::detach_memory() -> backing_t
	{
		auto tmp = memory_;
		memory_ = nullptr;
		return tmp;
	}

	using unique_memory_t = basic_unique_memory_t<atma::aligned_allocator_t<byte, 4>>;




	template <typename E>
	struct memory_view_t
	{
		template <typename T>
		memory_view_t(T&& c, size_t offset, size_t size)
			: begin_{reinterpret_cast<E*>(c.begin() + offset)}
			, end_{reinterpret_cast<E*>(c.begin() + offset + size)}
		{}

		template <typename T>
		memory_view_t(T&& c)
			: memory_view_t(c, 0, c.size())
		{}

		memory_view_t(E* begin, E* end)
			: begin_(begin)
			, end_(end)
		{}

		auto begin() const -> E* { return begin_; }
		auto end() const -> E* { return end_; }
		auto size() const -> size_t { return end() - begin(); }

		auto operator [](size_t idx) const -> E&
		{
			return begin_[idx];
		}

	private:
		E* begin_;
		E* end_;
	};
}



