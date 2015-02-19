#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

namespace atma
{
	struct unique_memory_t
	{
		explicit unique_memory_t();
		explicit unique_memory_t(size_t size);
		unique_memory_t(size_t size, void const* data);
		unique_memory_t(uint alignment, size_t size);
		unique_memory_t(uint alignment, size_t size, void const* data);
		unique_memory_t(unique_memory_t&&);
		~unique_memory_t();
		unique_memory_t(unique_memory_t const&) = delete;

		auto operator = (unique_memory_t&&) -> unique_memory_t&;
		auto operator = (unique_memory_t const&) -> unique_memory_t& = delete;

		operator bool() const;

		auto empty() const -> bool;
		auto size() const -> size_t;
		auto begin() const -> byte const*;
		auto end() const -> byte const*;
		template <typename T> auto begin() const -> T const*;
		template <typename T> auto end() const -> T const*;
		template <typename T> auto at(size_t) const -> T const&;

		auto begin() -> byte*;
		auto end() -> byte*;
		template <typename T> auto begin() -> T*;
		template <typename T> auto end() -> T*;
		template <typename T> auto at(size_t) -> T&;

		auto swap(unique_memory_t&) -> void;

	private:
		byte* begin_;
		byte* end_;
	};




	inline unique_memory_t::unique_memory_t()
		: begin_()
		, end_()
	{
		
	}

	inline unique_memory_t::unique_memory_t(size_t size)
		: unique_memory_t(4, size)
	{
	}

	inline unique_memory_t::unique_memory_t(size_t size, void const* data)
		: unique_memory_t(4, size, data)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, size_t size)
		: begin_(reinterpret_cast<byte*>(platform::allocate_aligned_memory(alignment, size)))
		, end_(begin_ + size)
	{
	}

	inline unique_memory_t::unique_memory_t(uint alignment, size_t size, void const* data)
		: begin_(reinterpret_cast<byte*>(platform::allocate_aligned_memory(alignment, size)))
		, end_(begin_ + size)
	{
		memcpy(begin_, data, size);
	}

	inline unique_memory_t::unique_memory_t(unique_memory_t&& rhs)
		: begin_(rhs.begin_)
		, end_(rhs.end_)
	{
		rhs.begin_ = nullptr;
		rhs.end_ = nullptr;
	}

	inline unique_memory_t::~unique_memory_t()
	{
		platform::deallocate_aligned_memory(begin_);
	}

	inline auto unique_memory_t::operator = (unique_memory_t&& rhs) -> unique_memory_t&
	{
		this->~unique_memory_t();
		begin_ = rhs.begin_;
		end_ = rhs.end_;
		rhs.begin_ = nullptr;
		rhs.end_ = nullptr;
		return *this;
	}

	inline unique_memory_t::operator bool() const
	{
		return begin_ != nullptr;
	}

	inline auto unique_memory_t::empty() const -> bool
	{
		return size() == 0;
	}

	inline auto unique_memory_t::size() const -> size_t
	{
		return end_ - begin_;
	}

	inline auto unique_memory_t::begin() const -> byte const*
	{
		return begin_;
	}

	inline auto unique_memory_t::end() const -> byte const*
	{
		return end_;
	}

	template <typename T>
	inline auto unique_memory_t::begin() const -> T const*
	{
		return reinterpret_cast<T const*>(begin_);
	}

	template <typename T>
	inline auto unique_memory_t::end() const -> T const*
	{
		return reinterpret_cast<T const*>(end_);
	}

	template <typename T>
	inline auto unique_memory_t::at(size_t i) const -> T const&
	{
		return reinterpret_cast<T*>(begin_)[i];
	}

	inline auto unique_memory_t::begin() -> byte*
	{
		return begin_;
	}

	inline auto unique_memory_t::end() -> byte*
	{
		return end_;
	}

	template <typename T>
	inline auto unique_memory_t::begin() -> T*
	{
		return reinterpret_cast<T*>(begin_);
	}

	template <typename T>
	inline auto unique_memory_t::end() -> T*
	{
		return reinterpret_cast<T*>(end_);
	}

	template <typename T>
	inline auto unique_memory_t::at(size_t i) -> T&
	{
		return reinterpret_cast<T*>(begin_)[i];
	}

	inline auto unique_memory_t::swap(unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}





	template <typename T, typename E>
	struct memory_view_t
	{
		memory_view_t(T const& c)
			: c_(c)
		{}

		auto begin() const -> E const* { return reinterpret_cast<E const*>(c_.begin()); }
		auto end() const -> E const* { return reinterpret_cast<E const*>(c_.end()); }

	private:
		T const& c_;
	};
}



