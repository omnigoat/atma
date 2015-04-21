#pragma once

#include <type_traits>

//
//  memory_t
//  -------------
//
//    this structure stores an allocator & pointer. this exists because we often
//    have an empty allocator (type with no members), so we can use Empty Base Class
//    Optimization. an interface is also provided for working with the memory, using
//    the stored allocator.
//
//    WHAT THIS CLASS DOES NOT DO
//
//      - this class doesn't manage the lifetime of the memory allocated within it.
//        it will NOT deallocate on destruction, will not deallocate if assigned 
//        another instance
// 
namespace atma
{
	namespace detail
	{
		template <typename Alloc, bool Empty = std::is_empty<Alloc>::value>
		struct base_memory_t;

		template <typename Alloc>
		struct base_memory_t<Alloc, false>
		{
			base_memory_t()
			{}

			base_memory_t(Alloc& allocator)
				: allocator_(allocator)
			{}

			base_memory_t(base_memory_t&& rhs)
				: allocator_(std::move(rhs.allocator_))
			{}

			auto allocator() -> Alloc& { return allocator_; }
			auto allocator() const -> Alloc const& { return allocator_; }

		private:
			Alloc allocator_;
		};

		template <typename Alloc>
		struct base_memory_t<Alloc, true>
			: protected Alloc
		{
			base_memory_t()
			{
			}

			base_memory_t(Alloc& allocator)
				: Alloc(allocator)
			{
			}

			base_memory_t(base_memory_t&& rhs)
				: Alloc(std::move(rhs))
			{}

			auto allocator() -> Alloc& { return static_cast<Alloc&>(*this); }
			auto allocator() const -> Alloc const& { return static_cast<Alloc const&>(*this); }
		};
	}




	template <typename Alloc>
	struct memory_t : detail::base_memory_t<Alloc>
	{
		using value_type = typename Alloc::value_type;

		memory_t();
		memory_t(memory_t const& rhs);
		explicit memory_t(Alloc& allocator);
		explicit memory_t(value_type* data);

		auto operator = (memory_t const& rhs) -> memory_t&;

		auto alloc(size_t capacity) -> void;
		auto deallocate() -> void;
		auto construct_default(size_t offset, size_t count) -> void;
		auto construct_copy(size_t offset, value_type const&) -> void;
		auto construct_copy_range(size_t offset, value_type const*, size_t size) -> void;
		auto construct_move(size_t offset, value_type&&) -> void;
		auto destruct(size_t offset, size_t count) -> void;

		auto memmove(size_t dest, size_t src, size_t count) -> void;
		auto memcpy(size_t dest, memory_t const&, size_t src, size_t count) -> void;

		auto detach_ptr() -> value_type*;

		value_type* ptr;
	};




	template <typename A>
	inline memory_t<A>::memory_t()
		: ptr()
	{}

	template <typename A>
	inline memory_t<A>::memory_t(A& allocator)
		: detail::base_memory_t(allocator)
		, ptr()
	{}

	template <typename A>
	inline memory_t<A>::memory_t(memory_t const& rhs)
		: detail::base_memory_t<A>(rhs)
		, ptr(rhs.ptr)
	{
	}

	template <typename A>
	inline memory_t<A>::memory_t(value_type* data)
		: ptr(data)
	{}

	template <typename A>
	inline auto memory_t<A>::operator = (memory_t const& rhs) -> memory_t&
	{
		allocator() = rhs.allocator();
		ptr = rhs.ptr;
		return *this;
	}

	template <typename A>
	inline auto memory_t<A>::alloc(size_t capacity) -> void
	{
		ptr = allocator().allocate(capacity);
	}

	template <typename A>
	inline auto memory_t<A>::deallocate() -> void
	{
		return allocator().deallocate(ptr, 0);
	}

	template <typename A>
	inline auto memory_t<A>::construct_default(size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().construct(ptr + i);
	}

	template <typename A>
	inline auto memory_t<A>::construct_copy(size_t offset, value_type const& x) -> void
	{
		allocator().construct(ptr + offset, x);
	}

	template <typename A>
	inline auto memory_t<A>::construct_copy_range(size_t offset, value_type const* rhs, size_t size) -> void
	{
		for (auto i = 0, j = offset; i != size; ++i, ++j)
			allocator().construct(ptr + j, rhs[i]);
	}

	template <typename A>
	inline auto memory_t<A>::construct_move(size_t offset, value_type&& rhs) -> void
	{
		allocator().construct(ptr + offset, std::move(rhs));
	}

	template <typename A>
	inline auto memory_t<A>::destruct(size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().destroy(ptr + i);
	}

	template <typename A>
	inline auto memory_t<A>::memmove(size_t dest, size_t src, size_t count) -> void
	{
		std::memmove(ptr + dest, ptr + src, sizeof(value_type) * count);
	}

	template <typename A>
	inline auto memory_t<A>::memcpy(size_t dest, memory_t const& rhs, size_t src, size_t count) -> void
	{
		std::memcpy(ptr + dest, rhs.ptr + src, sizeof(value_type) * count);
	}

	template <typename A>
	inline auto memory_t<A>::detach_ptr() -> value_type*
	{
		auto t = ptr;
		ptr = nullptr;
		return t;
	}
}
