#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

namespace atma
{
	struct unique_memory_t
	{
		explicit unique_memory_t();
		explicit unique_memory_t(size_t size);
		unique_memory_t(size_t size, void* data);
		unique_memory_t(uint alignment, size_t size);
		unique_memory_t(uint alignment, size_t size, void* data);
		unique_memory_t(unique_memory_t&&);
		~unique_memory_t();
		unique_memory_t(unique_memory_t const&) = delete;

		auto operator = (unique_memory_t&&) -> unique_memory_t&;
		auto operator = (unique_memory_t const&) -> unique_memory_t& = delete;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> void const*;
		auto end() const -> void const*;
		operator bool() const;

		auto begin() -> void*;
		auto end() -> void*;
		auto swap(unique_memory_t&) -> void;

	private:
		void* data_;
		void* end_;
	};




	inline unique_memory_t::unique_memory_t()
		: data_()
		, end_()
	{
		
	}

	inline unique_memory_t::unique_memory_t(size_t size)
		: unique_memory_t(4, size)
	{
	}

	inline unique_memory_t::unique_memory_t(size_t size, void* data)
		: unique_memory_t(4, size, data)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, size_t size)
		: data_(platform::allocate_aligned_memory(alignment, size))
		, end_(reinterpret_cast<char*>(data_) + size)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, size_t size, void* data)
		: data_(platform::allocate_aligned_memory(alignment, size))
		, end_(reinterpret_cast<char*>(data_) + size)
	{
		memcpy(data_, data, size);
	}

	inline unique_memory_t::unique_memory_t(unique_memory_t&& rhs)
		: data_(rhs.data_)
		, end_(rhs.end_)
	{
		rhs.data_ = nullptr;
		rhs.end_ = nullptr;
	}

	inline unique_memory_t::~unique_memory_t()
	{
		platform::deallocate_aligned_memory(data_);
	}

	inline auto unique_memory_t::operator = (unique_memory_t&& rhs) -> unique_memory_t&
	{
		this->~unique_memory_t();
		data_ = rhs.data_;
		end_ = rhs.end_;
		rhs.data_ = nullptr;
		rhs.end_ = nullptr;
		return *this;
	}

	inline auto unique_memory_t::empty() const -> bool
	{
		return size() == 0;
	}

	inline auto unique_memory_t::size() const -> size_t
	{
		return reinterpret_cast<char const*>(end_) - reinterpret_cast<char const*>(data_);
	}

	inline auto unique_memory_t::begin() -> void*
	{
		return data_;
	}

	inline auto unique_memory_t::end() -> void*
	{
		return end_;
	}

	inline auto unique_memory_t::begin() const -> void const*
	{
		return data_;
	}

	inline auto unique_memory_t::end() const -> void const*
	{
		return end_;
	}

	inline auto unique_memory_t::swap(unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}

	inline unique_memory_t::operator bool() const
	{
		return data_ != nullptr;
	}
}



