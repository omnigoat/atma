#pragma once

#include <atma/types.hpp>
#include <atma/memory.hpp>
#include <atma/aligned_allocator.hpp>


namespace atma
{
	constexpr struct unique_memory_allocate_copy_tag {} unique_memory_allocate_copy;
	constexpr struct unique_memory_take_ownership_tag {} unique_memory_take_ownership;


	template <typename T, typename Alloc>
	struct basic_unique_memory_t
	{
		using value_type = std::remove_cv_t<T>;
		using allocator_type = Alloc;
		using backing_t = atma::basic_memory_t<value_type, allocator_type>;

		basic_unique_memory_t(basic_unique_memory_t const&) = delete;
		basic_unique_memory_t(basic_unique_memory_t&&);
		~basic_unique_memory_t();

		template <typename U> basic_unique_memory_t(basic_unique_memory_t<T, U>&&);

		explicit basic_unique_memory_t(allocator_type const& = allocator_type());
		explicit basic_unique_memory_t(size_t size, allocator_type const& = allocator_type());
		basic_unique_memory_t(unique_memory_allocate_copy_tag, void const* data, size_t size_bytes);
		basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size_bytes);

		auto operator = (basic_unique_memory_t const&) -> basic_unique_memory_t& = delete;
		template <typename U> auto operator = (basic_unique_memory_t<T, U>&&) -> basic_unique_memory_t&;

		auto empty() const -> bool;
		auto size() const -> size_t;

		auto count() const -> size_t;

		auto begin() const -> value_type const*;
		auto end() const -> value_type const*;
		auto begin() -> value_type*;
		auto end() -> value_type*;

		auto reset(void* mem, size_t size) -> void;
		auto reset(size_t) -> void;

		auto swap(basic_unique_memory_t&) -> void;

		auto detach_memory() -> backing_t;

		auto memory_operations() -> backing_t& { return memory_; }

	private:
		backing_t memory_;
		size_t size_ = 0;

		template <typename, typename> friend struct basic_unique_memory_t;
	};



	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(allocator_type const& alloc)
		: memory_(alloc)
		, size_()
	{}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(size_t size, allocator_type const& alloc)
		: memory_(alloc)
		, size_(size)
	{
		if (size)
		{
			memory_.allocate(size);
		}
	}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(unique_memory_allocate_copy_tag, void const* data, size_t size)
		: size_(size)
	{
		if (size)
		{
			memory_.allocate(size);
			memory_.memcpy(0, data, size);
		}
	}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size)
		: memory_{data}
		, size_(size)
	{
	}

	template <typename T, typename A>
	template <typename B>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(basic_unique_memory_t<T, B>&& rhs)
		: memory_{rhs.memory_}
		, size_{rhs.size_}
	{
		rhs.memory_ = nullptr;
		rhs.size_ = 0;
	}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::~basic_unique_memory_t()
	{
		memory_.deallocate();
	}

	template <typename T, typename A>
	template <typename U>
	inline auto basic_unique_memory_t<T, A>::operator = (basic_unique_memory_t<T, U>&& rhs) -> basic_unique_memory_t&
	{
		this->~basic_unique_memory_t();
		memory_ = rhs.memory_;
		size_ = rhs.size_;
		rhs.memory_ = nullptr;
		return *this;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::empty() const -> bool
	{
		return size() == 0;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::size() const -> size_t
	{
		return size_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::count() const -> size_t
	{
		return size_ / sizeof(T);
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::begin() const -> value_type const*
	{
		return memory_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::end() const -> value_type const*
	{
		return memory_ + size_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::begin() -> value_type*
	{
		return memory_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::end() -> value_type*
	{
		return memory_ + size_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::reset(void* mem, size_t size) -> void
	{
		memory_.deallocate();
		memory_.allocate(size);
		memory_.memcpy(0, mem, size);
		size_ = size;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::reset(size_t size) -> void
	{
		if (size != size_)
		{
			memory_.deallocate();
			memory_.allocate(size);
			size_ = size;
		}
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::swap(basic_unique_memory_t& rhs) -> void
	{
		auto tmp = std::move(*this);
		*this = std::move(rhs);
		rhs = std::move(tmp);
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::detach_memory() -> backing_t
	{
		auto tmp = memory_;
		memory_ = nullptr;
		return tmp;
	}

	using unique_memory_t = basic_unique_memory_t<byte, atma::aligned_allocator_t<byte, 4>>;

	template <typename T>
	using typed_unique_memory_t = basic_unique_memory_t<T, atma::aligned_allocator_t<byte, 4>>;



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

		auto size() const -> size_t { return end_ - begin_; }
		auto begin() const -> E* { return begin_; }
		auto end() const -> E* { return end_; }

		auto operator [](size_t idx) const -> E&
		{
			return begin_[idx];
		}

	private:
		E* begin_;
		E* end_;
	};

	template <typename R>
	memory_view_t(R&& range) -> memory_view_t<typename std::remove_reference_t<R>::value_type>;
}



