#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>
#include <atma/allocdata.hpp>

namespace atma
{
	struct unique_memory_take_ownership_tag {};

	template <typename Alloc>
	struct basic_unique_memory_t
	{
		basic_unique_memory_t();
		explicit basic_unique_memory_t(size_t size);
		basic_unique_memory_t(size_t size, void const* data);
		basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size);
		basic_unique_memory_t(basic_unique_memory_t const&) = delete;
		basic_unique_memory_t(basic_unique_memory_t&&);
		~basic_unique_memory_t();

		auto operator = (basic_unique_memory_t const&) -> basic_unique_memory_t& = delete;
		auto operator = (basic_unique_memory_t&&) -> basic_unique_memory_t&;

		operator bool() const;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> byte const*;
		auto end() const -> byte const*;
		auto begin() -> byte*;
		auto end() -> byte*;

		auto swap(basic_unique_memory_t&) -> void;

	private:
		atma::allocdata_t<Alloc> memory_;
		size_t size_;
	};



	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t()
		: begin_()
		, end_()
	{
		
	}


	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(size_t size)
		: size_(size)
	{
		memory_.alloc(size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(size_t size, void const* data)
		: size_(size)
	{
		memory_.alloc(size);
		memcpy(memory_.data(), data, size);
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size)
		: memory_{data}
		, size_(size)
	{
	}

	template <typename A>
	inline basic_unique_memory_t<A>::basic_unique_memory_t(basic_unique_memory_t&& rhs)
		: memory_(std::move(rhs.memory_))
		, size_(rhs.size_)
	{
		rhs.size_ = 0;
	}

	template <typename A>
	inline basic_unique_memory_t<A>::~basic_unique_memory_t()
	{
		memory_.deallocate();
	}

	template <typename A>
	inline auto basic_unique_memory_t<A>::operator = (basic_unique_memory_t&& rhs) -> basic_unique_memory_t&
	{
		this->~basic_unique_memory_t();
		begin_ = rhs.begin_;
		end_ = rhs.end_;
		rhs.begin_ = nullptr;
		rhs.end_ = nullptr;
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
	inline auto basic_unique_memory_t<A>::swap(basic_unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
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



