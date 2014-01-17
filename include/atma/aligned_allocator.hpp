#ifndef ATMA_ALIGNED_ALLOCATOR_HPP
#define ATMA_ALIGNED_ALLOCATOR_HPP
//======================================================================
#include <cstdint>
//======================================================================
namespace atma {
//======================================================================
	
	namespace detail {
		void* allocate_aligned_memory(uint32_t alignment, uint32_t size);
		void* deallocate_aligned_memory(void*);
	}

	template <typename T, uint32_t A>
	struct aligned_allocator_t;


	template <uint32_t A>
	struct aligned_allocator_t<void, A>
	{
		typedef void* pointer;
		typedef void const* const_pointer;
		typedef void value_type;

		template <typename U> struct rebind { typedef aligned_allocator_t<U, A> other_t; };
	};

	template <typename T, uint32_t A>
	struct aligned_allocator_t
	{
		typedef T value_type;
		typedef T* pointer;
		typedef T const* const_pointer;
		typedef T& reference;
		typedef T const& const_reference;
		typedef uint32_t size_type;
		typedef ptrdiff_t difference_type;

		typedef std::true_type propagate_on_container_move_assignment_t;

		template <typename U>
		struct rebind { typedef aligned_allocator_t<U, A> other_t; };

		aligned_allocator_t() {}
		template <typename U> aligned_allocator_t(aligned_allocator_t<U, A> const&) {}

		auto max_size() const -> size_type;
		auto address(reference x) const -> pointer { return std::addressof(x); }
		auto address(const_reference x) const -> const_pointer { return std::addressof(x); }
		auto allocate(size_type n, typename aligned_allocator_t<void, A>::const_pointer = 0) -> pointer
		{
			size_type const alignment = static_cast<size_type>(A);
			void* ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
			if (ptr == nullptr) {
				throw std::bad_alloc();
			}

			return reinterpret_cast<pointer>(ptr);
		}

		auto deallocate(pointer p, size_type) -> void { return detail::deallocate_aligned_memory(p); }

		template <typename U, class... Args>
		auto construct(U* p, Args&&... args) -> void
		{
			::new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
		}

		auto destroy(pointer p) -> void { p->~T(); }
	};


	//======================================================================
	// IMPLEMENTATION
	//======================================================================
	template <typename T, uint32_t A>
	auto aligned_allocator_t<T, A>::max_size() const -> size_type {
		return (size_type(~0) - size_type(A)) / sizeof(T);
	}

	template <typename T, uint32_t A>
	auto aligned_allocator_t<T, A>::address(reference x) const -> pointer {
		return std::addressof(x);
	}

	template <typename T, uint32_t A>
	auto aligned_allocator_t<T, A>::address(const_reference x) const -> const_pointer {
		return std::addressof(x);
	}

	template <typename T, uint32_t A>
	auto aligned_allocator_t<T, A>::allocate(size_type n, typename aligned_allocator_t<void, A>::const_pointer = 0) -> pointer
	{
		size_type const alignment = static_cast<size_type>(A);
		void* ptr = detail::allocate_aligned_memory(alignment, n * sizeof(T));
		if (ptr == nullptr) {
			throw std::bad_alloc();
		}

		return reinterpret_cast<pointer>(ptr);
	}


//======================================================================
} // namespace atma
//======================================================================
#endif
//======================================================================
