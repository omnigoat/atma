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
		basic_unique_memory_t();
		explicit basic_unique_memory_t(size_t size);
		basic_unique_memory_t(void const* data, size_t size);
		basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size);
		basic_unique_memory_t(basic_unique_memory_t const&) = delete;
		template <typename U> basic_unique_memory_t(basic_unique_memory_t<U>&&);
		~basic_unique_memory_t();

		auto operator = (basic_unique_memory_t const&) -> basic_unique_memory_t& = delete;
		template <typename U> auto operator = (basic_unique_memory_t<U>&&) -> basic_unique_memory_t&;

		operator bool() const;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> byte const*;
		auto end() const -> byte const*;
		auto begin() -> byte*;
		auto end() -> byte*;

		auto swap(basic_unique_memory_t&) -> void;

		auto detach_memory() -> atma::memory_t<byte, Alloc>;

	private:
		atma::memory_t<byte, Alloc> memory_;
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
		memory_allocate(memory_, size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(void const* data, size_t size)
		: size_(size)
	{
		memory_allocate(memory_, size);
		memcpy(memory_.ptr, data, size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size)
		: memory_{}
		, size_(size)
	{
		memory_.ptr = reinterpret_cast<byte*>(data);
	}

	template <typename A>
	template <typename U>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(basic_unique_memory_t<U>&& rhs)
		: memory_{rhs.memory_}
		, size_{rhs.size_}
	{
		rhs.memory_.ptr = nullptr;
		rhs.size_ = 0;
	}

	template <typename A>
	inline basic_unique_memory_t<A>::~basic_unique_memory_t()
	{
		memory_deallocate(memory_);
	}

	template <typename A>
	template <typename U>
	inline auto basic_unique_memory_t<A>::operator = (basic_unique_memory_t<U>&& rhs) -> basic_unique_memory_t&
	{
		this->~basic_unique_memory_t();
		memory_ = rhs.memory_;
		size_ = rhs.size_;
		rhs.memory_.ptr = nullptr;
		return *this;
	}

	template <typename A>
	inline basic_unique_memory_t<A>::operator bool() const
	{
		return begin_ != nullptr;
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
		return memory_.ptr;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::end() const -> byte const*
	{
		return memory_.ptr + size_;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::begin() -> byte*
	{
		return memory_.ptr;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::end() -> byte*
	{
		return memory_.ptr + size_;
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::swap(basic_unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::detach_memory() -> memory_t<byte, A>
	{
		auto tmp = memory_t<byte, A>{memory_};
		memory_.ptr = nullptr;
		return tmp;
	}

	using unique_memory_t = basic_unique_memory_t<atma::aligned_allocator_t<byte, 4>>;




	template <typename T, typename E>
	struct memory_view_t
	{
		memory_view_t(T& c, size_t offset, size_t size)
			: begin_{c.begin() + offset}, end_{c.begin() + offset + size}
		{}

		memory_view_t(T& c)
			: memory_view_t(c, 0, c.size())
		{}

		auto begin() const -> E* { return reinterpret_cast<E*>(begin_); }
		auto end() const -> E* { return reinterpret_cast<E*>(end_); }
		auto size() const -> size_t { return end() - begin(); }

		auto operator [](size_t idx) const -> E&
		{
			return begin()[idx];
		}

	private:
		using storage_ptr = std::conditional_t<std::is_const<T>::value, void const*, void*>;

		storage_ptr begin_, end_;
	};
}



