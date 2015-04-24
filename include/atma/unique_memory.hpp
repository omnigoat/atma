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

		auto detach_memory() -> atma::memory_t<Alloc>;

	private:
		atma::memory_t<typename Alloc::template rebind<byte>::other> memory_;
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
		memory_.alloc(size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(void const* data, size_t size)
		: size_(size)
	{
		memory_.alloc(size);
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
		: memory_{rhs.memory_.detach_ptr()}
		, size_{rhs.size_}
	{
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
	inline auto basic_unique_memory_t<A>::detach_memory() -> memory_t<A>
	{
		auto tmp = memory_;
		memory_.detach_ptr();
		return tmp;
	}

	using unique_memory_t = basic_unique_memory_t<atma::aligned_allocator_t<byte, 4>>;




	template <typename T, typename E>
	struct memory_view_t
	{
		memory_view_t(T& c)
			: c_(c)
		{}

		auto begin() const -> E* { return reinterpret_cast<E*>(c_.begin()); }
		auto end() const -> E* { return reinterpret_cast<E*>(c_.end()); }

	private:
		T& c_;
	};
}



