#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

namespace atma
{
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
		auto begin() -> char*;
		auto end() -> char*;
		auto begin() const -> char const*;
		auto end() const -> char const*;

	private:
		auto internal_size() -> size_t;
		auto decrement() -> void;
		auto increment() -> void;

		auto ref() -> std::atomic_uint32_t&;

	private:
		byte* data_ = nullptr;
	};




	inline shared_memory_t::shared_memory_t(size_t size)
		: shared_memory_t(4, size)
	{}

	inline shared_memory_t::shared_memory_t(size_t size, void* data)
		: shared_memory_t(4, size, data)
	{}

	inline shared_memory_t::shared_memory_t(size_t alignment, size_t size)
		: data_((char*)platform::allocate_aligned_memory(alignment, size + sizeof(std::atomic_size_t)))
	{
		new (&ref()) std::atomic_size_t{1};
	}

	inline shared_memory_t::shared_memory_t(size_t alignment, size_t size, void* data)
		: data_((char*)platform::allocate_aligned_memory(alignment, size + sizeof(std::atomic_size_t)))
		, ref_(new (data_ + size) std::atomic_size_t{1})
	{
		memcpy(data_, data, size);
	}

	inline shared_memory_t::shared_memory_t(shared_memory_t const& rhs)
		: data_(rhs.data_), ref_(rhs.ref_)
	{
		increment();
	}

	inline shared_memory_t::shared_memory_t(shared_memory_t&& rhs)
		: data_(rhs.data_), ref_(rhs.ref_)
	{
		rhs.data_ = nullptr;
		rhs.ref_ = nullptr;
	}

	inline shared_memory_t::~shared_memory_t()
	{
		decrement();
	}

	inline auto shared_memory_t::operator = (shared_memory_t const& rhs) -> shared_memory_t&
	{
		decrement();
		data_ = rhs.data_;
		ref_ = rhs.ref_;
		increment();
		return *this;
	}

	inline auto shared_memory_t::operator = (shared_memory_t&& rhs) -> shared_memory_t&
	{
		std::swap(data_, rhs.data_);
		std::swap(ref_, rhs.ref_);
		data_ = rhs.data_;
		ref_ = rhs.ref_;
		increment();
		return *this;
	}

	inline auto shared_memory_t::size() const -> size_t
	{
		return reinterpret_cast<char*>(ref_) - data_;
	}

	inline auto shared_memory_t::begin() -> char*
	{
		return data_;
	}

	inline auto shared_memory_t::end() -> char*
	{
		return reinterpret_cast<char*>(ref_);
	}

	inline auto shared_memory_t::begin() const -> char const*
	{
		return data_;
	}

	inline auto shared_memory_t::end() const -> char const*
	{
		return reinterpret_cast<char const*>(ref_);
	}

	inline auto shared_memory_t::internal_size() const -> size_t
	{
		return sizeof(size_t) + size() + sizeof(std::atomic_size_t);
	}

	inline auto shared_memory_t::decrement() -> void
	{
		if (ref_ && --*ref_ == 0)
		{
			atma::platform::deallocate_aligned_memory(data_);
			data_ = nullptr;
			ref_ = nullptr;
		}
	}

	inline auto shared_memory_t::increment() -> void
	{
		if (ref_)
			++*ref_;
	}

}



