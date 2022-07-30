module;

#include <atma/platform/allocation.hpp>
#include <exception>

export module atma.aligned_allocator;

import atma.types;

export namespace atma
{
	template <typename T, size_t A = alignof(T)>
	struct aligned_allocator_t
	{
		using size_type       = size_t;
		using difference_type = ptrdiff_t;
		using value_type      = T;
		using pointer         = value_type*;
		using const_pointer   = value_type const*;

		// must provide rebind because of non-type template parameter A
		template <typename U>
		struct rebind { using other = aligned_allocator_t<U, A>; };

		aligned_allocator_t() noexcept
		{}

		template <typename U>
		aligned_allocator_t(aligned_allocator_t<U, A> const&) noexcept
		{}

		auto allocate(size_type n) -> pointer
		{
			void* ptr = platform::allocate_aligned_memory(A, n * sizeof(T));
			if (ptr == nullptr)
			{
				throw std::bad_alloc();
			}

			return reinterpret_cast<pointer>(ptr);
		}

		auto deallocate(pointer p, size_type) -> void
		{
			platform::deallocate_aligned_memory(p);
		}
	};


	template <typename T, size_t TA>
	inline bool operator == (aligned_allocator_t<T, TA> const&, aligned_allocator_t<T, TA> const&)
	{
		return true;
	}

	template <typename T, size_t TA>
	inline bool operator != (aligned_allocator_t<T, TA> const&, aligned_allocator_t<T, TA> const&)
	{
		return false;
	}
}
