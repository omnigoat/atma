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
//        another instance, and shoot you in your face if you mismanage your memory.
// 
namespace atma
{
	namespace detail
	{
		template <
			typename Alloc,
			bool Empty = std::is_empty<Alloc>::value
		>
		struct base_memory_t;


		template <typename Alloc>
		struct base_memory_t<Alloc, false>
		{
			base_memory_t()
			{}

			template <typename U>
			base_memory_t(base_memory_t<U> const& rhs)
				: allocator_(rhs.allocator())
			{}

			base_memory_t(Alloc const& allocator)
				: allocator_(allocator)
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
			{}

			template <typename U>
			base_memory_t(base_memory_t<U> const& rhs)
				: Alloc(rhs.allocator())
			{}

			base_memory_t(Alloc const& allocator)
				: Alloc(allocator)
			{}

			auto allocator() -> Alloc& { return static_cast<Alloc&>(*this); }
			auto allocator() const -> Alloc const& { return static_cast<Alloc const&>(*this); }
		};
	}




	template <typename T, typename Allocator>
	struct memory_t : detail::base_memory_t<typename Allocator::template rebind<T>::other>
	{
		using value_type = T;

		memory_t();
		memory_t(memory_t&&);
		template <typename U> explicit memory_t(memory_t<T, U> const&);
		explicit memory_t(Allocator const& allocator);
		explicit memory_t(value_type* data, Allocator const& = Allocator());
		memory_t(memory_t const&) = delete;

		auto operator = (memory_t const&) -> memory_t& = delete;

		auto alloc(size_t capacity) -> void;
		auto deallocate() -> void;
		auto construct_default(size_t offset, size_t count, value_type const& = value_type()) -> void;
		auto construct_copy(size_t offset, value_type const&) -> void;
		auto construct_copy_range(size_t offset, value_type const*, size_t size) -> void;
		auto construct_move(size_t offset, value_type&&) -> void;
		auto destruct(size_t offset, size_t count) -> void;

		auto memmove(size_t dest, size_t src, size_t count) -> void;
		template <typename U> auto memcpy(size_t dest, memory_t<T,U> const&, size_t src, size_t count) -> void;

		auto detach_ptr() -> value_type*;

		value_type* ptr;
	};




	template <typename T, typename A>
	inline memory_t<T,A>::memory_t()
		: ptr()
	{}

	template <typename T, typename A>
	inline memory_t<T,A>::memory_t(A const& allocator)
		: detail::base_memory_t(allocator)
		, ptr()
	{}

	template <typename T, typename A>
	inline memory_t<T,A>::memory_t(memory_t&& rhs)
		: detail::base_memory_t<A>(rhs)
		, ptr(rhs.ptr)
	{
		rhs.ptr = nullptr;
	}

	template <typename T, typename A>
	inline memory_t<T,A>::memory_t(value_type* data, A const& alloc)
		: detail::base_memory_t<A>(alloc)
		, ptr(data)
	{}

	template <typename T, typename A>
	inline auto memory_t<T,A>::alloc(size_t capacity) -> void
	{
		ptr = allocator().allocate(capacity);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::deallocate() -> void
	{
		return allocator().deallocate(ptr, 0);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::construct_default(size_t offset, size_t count, value_type const& x) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().construct(ptr + i, x);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::construct_copy(size_t offset, value_type const& x) -> void
	{
		allocator().construct(ptr + offset, x);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::construct_copy_range(size_t offset, value_type const* rhs, size_t size) -> void
	{
		for (auto i = size_t{}, j = offset; i != size; ++i, ++j)
			allocator().construct(ptr + j, rhs[i]);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::construct_move(size_t offset, value_type&& rhs) -> void
	{
		allocator().construct(ptr + offset, std::move(rhs));
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::destruct(size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			allocator().destroy(ptr + i);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::memmove(size_t dest, size_t src, size_t count) -> void
	{
		std::memmove(ptr + dest, ptr + src, sizeof(value_type) * count);
	}

	template <typename T, typename A>
	template <typename U>
	inline auto memory_t<T,A>::memcpy(size_t dest, memory_t<T, U> const& rhs, size_t src, size_t count) -> void
	{
		std::memcpy(ptr + dest, rhs.ptr + src, sizeof(T) * count);
	}

	template <typename T, typename A>
	inline auto memory_t<T,A>::detach_ptr() -> value_type*
	{
		auto t = ptr;
		ptr = nullptr;
		return t;
	}
}
