#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

namespace atma
{
	namespace detail
	{
		constexpr size_t header_size = sizeof(size_t) + sizeof(std::atomic_uint32_t) + 4u;

		constexpr inline size_t allocation_size(size_t alignment, size_t size)
		{
			return header_size + (header_size < alignment ? alignment - header_size : 0) + size;
		}
	}


	struct shared_memory_t
	{
		shared_memory_t() = default;
		explicit shared_memory_t(size_t size);
		explicit shared_memory_t(size_t size, void* data);
		explicit shared_memory_t(size_t alignment, size_t size);
		explicit shared_memory_t(size_t alignment, size_t size, void* data);
		shared_memory_t(shared_memory_t const&);
		shared_memory_t(shared_memory_t&&);
		~shared_memory_t();

		auto operator = (shared_memory_t const&) -> shared_memory_t&;
		auto operator = (shared_memory_t&&) -> shared_memory_t&;

		auto size() const -> size_t;
		auto begin() -> byte*;
		auto end() -> byte*;
		auto begin() const -> byte const*;
		auto end() const -> byte const*;

	private:
		auto decrement() -> void;
		auto increment() -> void;

		auto ref() -> std::atomic_uint32_t&;
		auto ref() const -> std::atomic_uint32_t const&;

	private:
		byte* data_ = nullptr;
	};

	// our actual data will start 16 bytes after the allocation, allowing space for
	// an 8-byte size information, a 4-byte atomic uint32_t for ref-counting, and 4 bytes of padding,
	// (to allow for 16-byte alignment naturally).
	static_assert(sizeof(std::atomic_uint32_t) == sizeof(uint32_t), "unexpected size of std::atomic");


	inline shared_memory_t::shared_memory_t(size_t size)
		: shared_memory_t(alignof(int), size)
	{}

	inline shared_memory_t::shared_memory_t(size_t size, void* data)
		: shared_memory_t(alignof(int), size, data)
	{}

	inline shared_memory_t::shared_memory_t(size_t alignment, size_t size)
		: data_((byte*)platform::allocate_aligned_memory(alignment, detail::allocation_size(alignment, size)))
	{
		new (data_) size_t{size};
		new (&ref()) std::atomic_uint32_t{1};
	}

	inline shared_memory_t::shared_memory_t(size_t alignment, size_t size, void* data)
		: data_((byte*)platform::allocate_aligned_memory(alignment, detail::allocation_size(alignment, size)))
	{
		new (data_) size_t{size};
		new (&ref()) std::atomic_uint32_t{1};
		memcpy(begin(), data, size);
	}

	inline shared_memory_t::shared_memory_t(shared_memory_t const& rhs)
		: data_(rhs.data_)
	{
		increment();
	}

	inline shared_memory_t::shared_memory_t(shared_memory_t&& rhs)
	{
		std::swap(data_, rhs.data_);
	}

	inline shared_memory_t::~shared_memory_t()
	{
		decrement();
	}

	inline auto shared_memory_t::operator = (shared_memory_t const& rhs) -> shared_memory_t&
	{
		if (this != &rhs)
		{
			data_ = rhs.data_;
		}

		return *this;
	}

	inline auto shared_memory_t::operator = (shared_memory_t&& rhs) -> shared_memory_t&
	{
		if (this != &rhs)
		{
			std::swap(data_, rhs.data_);
		}

		return *this;
	}

	inline auto shared_memory_t::size() const -> size_t
	{
		return *reinterpret_cast<size_t*>(data_);
	}

	inline auto shared_memory_t::begin() -> byte*
	{
		return data_ + sizeof(size_t) + sizeof(std::atomic_uint32_t) + 4u;
	}

	inline auto shared_memory_t::end() -> byte*
	{
		return begin() + size();
	}

	inline auto shared_memory_t::begin() const -> byte const*
	{
		return data_ + sizeof(size_t) + sizeof(std::atomic_uint32_t) + 4u;
	}

	inline auto shared_memory_t::end() const -> byte const*
	{
		return begin() + size();
	}

	inline auto shared_memory_t::decrement() -> void
	{
		if (data_ && --ref() == 0)
		{
			atma::platform::deallocate_aligned_memory(data_);
			data_ = nullptr;
		}
	}

	inline auto shared_memory_t::increment() -> void
	{
		if (data_)
			++ref();
	}

	inline auto shared_memory_t::ref() -> std::atomic_uint32_t&
	{
		return *reinterpret_cast<std::atomic_uint32_t*>(data_ + sizeof(size_t));
	}

	inline auto shared_memory_t::ref() const -> std::atomic_uint32_t const&
	{
		return *reinterpret_cast<std::atomic_uint32_t const*>(data_ + sizeof(size_t));
	}

}



