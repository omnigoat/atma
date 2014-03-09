#pragma once
//=====================================================================
#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	struct shared_memory
	{
		explicit shared_memory();
		explicit shared_memory(uint size);
		shared_memory(uint size, void* data);
		shared_memory(uint alignment, uint size);
		shared_memory(uint alignment, uint size, void* data);
		shared_memory(shared_memory const&);
		shared_memory(shared_memory&&);
		~shared_memory();

		auto operator = (shared_memory const&) -> shared_memory&;

		auto size() const -> uint;
		auto begin() -> char*;
		auto end() -> char*;
		auto begin() const -> char const*;
		auto end() const -> char const*;

		template <typename T>
		auto ref_at_as(uint offset) -> T& { 
			return *(T*)&data_[offset];
		}

		template <typename T>
		auto ref_at_as(uint offset) const -> T const& {
			return *(T const*)&data_[offset];
		}

	private:
		auto decrement() -> void;
		auto increment() -> void;

	private:
		char* data_;
		uint* ref_;
	};


	inline shared_memory::shared_memory()
		: data_(), ref_()
	{
		
	}

	inline shared_memory::shared_memory(uint size)
		: data_(new char[size + sizeof(int)]),
		  ref_((uint*)(data_ + size))
	{
		// set refcount to 1
		*ref_ = 1;
	}

	inline shared_memory::shared_memory(uint size, void* data)
		: data_(new char[size + sizeof(int)]),
		  ref_((uint*)(data_ + size))
	{
		// set refcount to 1
		*ref_ = 1;

		memcpy(data_, data, size);
	}

	inline shared_memory::shared_memory(uint alignment, uint size)
		: data_((char*)platform::allocate_aligned_memory(alignment, size + sizeof(int))),
		  ref_((uint*)(data_ + size))
	{
		// set refcount to 1
		*ref_ = 1;
	}

	inline shared_memory::shared_memory(uint alignment, uint size, void* data)
		: data_((char*)platform::allocate_aligned_memory(alignment, size + sizeof(int))),
		  ref_((uint*)(data_ + size))
	{
		// set refcount to 1
		*ref_ = 1;

		memcpy(data_, data, size);
	}

	inline shared_memory::shared_memory(shared_memory const& rhs)
		: data_(rhs.data_), ref_(rhs.ref_)
	{
		increment();
	}

	inline shared_memory::shared_memory(shared_memory&& rhs)
		: data_(rhs.data_), ref_(rhs.ref_)
	{
		rhs.data_ = nullptr;
		rhs.ref_ = nullptr;
	}

	inline shared_memory::~shared_memory()
	{
		decrement();
	}

	inline auto shared_memory::operator = (shared_memory const& rhs) -> shared_memory&
	{
		decrement();
		data_ = rhs.data_;
		ref_ = rhs.ref_;
		increment();
		return *this;
	}

	inline auto shared_memory::size() const -> uint
	{
		return reinterpret_cast<char*>(ref_) - data_;
	}

	inline auto shared_memory::begin() -> char*
	{
		return data_;
	}

	inline auto shared_memory::end() -> char*
	{
		return reinterpret_cast<char*>(ref_);
	}

	inline auto shared_memory::begin() const -> char const*
	{
		return data_;
	}

	inline auto shared_memory::end() const -> char const*
	{
		return reinterpret_cast<char const*>(ref_);
	}

	inline auto shared_memory::decrement() -> void
	{
		if (ref_ && --*ref_ == 0)
		{
			delete [] data_;
			data_ = nullptr;
			ref_ = nullptr;
		}
	}

	inline auto shared_memory::increment() -> void
	{
		if (ref_)
			++*ref_;
	}

//=====================================================================
}
//=====================================================================


