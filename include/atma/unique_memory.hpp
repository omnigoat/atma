#pragma once
//=====================================================================
#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>
//=====================================================================
namespace atma
{
	struct unique_memory_t
	{
		explicit unique_memory_t();
		explicit unique_memory_t(uint size);
		unique_memory_t(uint size, void* data);
		unique_memory_t(uint alignment, uint size);
		unique_memory_t(uint alignment, uint size, void* data);
		unique_memory_t(unique_memory_t&&);
		~unique_memory_t();

		unique_memory_t(unique_memory_t const&) = delete;
		auto operator = (unique_memory_t const&) -> unique_memory_t& = delete;

		auto operator *() -> void*;
		auto operator *() const -> void const*;

		auto size() const -> uint;
		auto begin() -> void*;
		auto end() -> void*;
		auto begin() const -> void const*;
		auto end() const -> void const*;

	private:
		void* data_;
		void* end_;
	};


	inline unique_memory_t::unique_memory_t()
		: data_(), end_()
	{
		
	}

	inline unique_memory_t::unique_memory_t(uint size)
		: unique_memory_t(4, size)
	{
	}

	inline unique_memory_t::unique_memory_t(uint size, void* data)
		: unique_memory_t(4, size, data)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, uint size)
		: data_(platform::allocate_aligned_memory(alignment, size)), end_(reinterpret_cast<char*>(data_) + size)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, uint size, void* data)
		: data_(platform::allocate_aligned_memory(alignment, size)), end_(reinterpret_cast<char*>(data_)+ size)
	{
		memcpy(data_, data, size);
	}

	inline unique_memory_t::unique_memory_t(unique_memory_t&& rhs)
		: data_(rhs.data_), end_(rhs.end_)
	{
		rhs.data_ = nullptr;
		rhs.end_ = nullptr;
	}

	inline unique_memory_t::~unique_memory_t()
	{
		platform::deallocate_aligned_memory(data_);
	}

	inline auto unique_memory_t::size() const -> uint
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
}



