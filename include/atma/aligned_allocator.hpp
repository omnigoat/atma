#pragma once

#include <atma/types.hpp>
#include <atma/platform/allocation.hpp>

#include <cstdint>


namespace atma
{
	template <typename T, size_t A = alignof(T)>
	struct aligned_allocator_t
	{
		using size_type       = size_t;
		using difference_type = ptrdiff_t;
		using value_type      = T;
		using pointer         = value_type*;
		using const_pointer   = value_type const*;

		// must provide rebind because of non-type template paramter A
		template <typename U>
		struct rebind { typedef aligned_allocator_t<U, A> other; };

		aligned_allocator_t() {}

		template <typename U>
		aligned_allocator_t(aligned_allocator_t<U, A> const&) {}

		auto allocate(size_type n) -> pointer;
		auto deallocate(pointer p, size_type) -> void;
	};

	template <size_t A>
	struct aligned_allocator_t<void, A>
	{
		using value_type = void;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		template <typename U>
		struct rebind { typedef aligned_allocator_t<U, A> other; };
	};


	template <typename T, size_t A>
	inline auto aligned_allocator_t<T, A>::allocate(size_type n) -> pointer
	{
		void* ptr = platform::allocate_aligned_memory(A, n * sizeof(T));
		if (ptr == nullptr)
		{
			throw std::bad_alloc();
		}

		return reinterpret_cast<pointer>(ptr);
	}

	template <typename T, size_t A>
	inline auto aligned_allocator_t<T, A>::deallocate(pointer p, size_type) -> void
	{
		platform::deallocate_aligned_memory(p);
	}


	template <typename T, size_t TA, typename U, size_t UA>
	inline bool operator == (aligned_allocator_t<T, TA> const&, aligned_allocator_t<U, UA> const&)
	{
		return TA == UA;
	}

	template <typename T, size_t TA, typename U, size_t UA>
	inline bool operator != (aligned_allocator_t<T, TA> const&, aligned_allocator_t<U, UA> const&)
	{
		return TA != UA;
	}
}
