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
		struct base_memory_tx;


		template <typename Alloc>
		struct base_memory_tx<Alloc, false>
		{
			base_memory_tx()
			{}

			template <typename U>
			base_memory_tx(base_memory_tx<U> const& rhs)
				: allocator_(rhs.allocator())
			{}

			base_memory_tx(Alloc const& allocator)
				: allocator_(allocator)
			{}

			auto allocator() -> Alloc& { return allocator_; }
			auto allocator() const -> Alloc const& { return allocator_; }

		private:
			Alloc allocator_;
		};


		template <typename Alloc>
		struct base_memory_tx<Alloc, true>
			: protected Alloc
		{
			base_memory_tx()
			{}

			template <typename U>
			base_memory_tx(base_memory_tx<U> const& rhs)
				: Alloc(rhs.allocator())
			{}

			base_memory_tx(Alloc const& allocator)
				: Alloc(allocator)
			{}

			auto allocator() -> Alloc& { return static_cast<Alloc&>(*this); }
			auto allocator() const -> Alloc const& { return static_cast<Alloc const&>(*this); }
		};


		template <typename T, typename A>
		using base_memory_t = base_memory_tx<typename A::template rebind<T>::other>;
	}




	template <typename T, typename Allocator>
	struct memory_t : detail::base_memory_t<T, Allocator>
	{
		using value_type = T;

		memory_t();
		template <typename B> memory_t(memory_t<T, B> const&);
		explicit memory_t(Allocator const& allocator);
		explicit memory_t(value_type* data, Allocator const& = Allocator());

		template <typename B> auto operator = (memory_t<T, B> const&) -> memory_t&;

		value_type* ptr;
	};




	template <typename T, typename A>
	inline memory_t<T,A>::memory_t()
		: ptr()
	{}

	template <typename T, typename A>
	inline memory_t<T,A>::memory_t(A const& allocator)
		: detail::base_memory_t<T, A>(allocator)
		, ptr()
	{}

	template <typename T, typename A>
	template <typename B>
	inline memory_t<T,A>::memory_t(memory_t<T, B> const& rhs)
		: detail::base_memory_t<T, A>(rhs.allocator())
		, ptr(rhs.ptr)
	{
		rhs.ptr = nullptr;
	}

	template <typename T, typename A>
	inline memory_t<T,A>::memory_t(value_type* data, A const& alloc)
		: detail::base_memory_t<T, A>(alloc)
		, ptr(data)
	{}

	template <typename T, typename A>
	template <typename B>
	inline auto memory_t<T, A>::operator = (memory_t<T, B> const& rhs) -> memory_t&
	{
		ptr = nullptr;
		allocator() = rhs.allocator();
		return *this;
	}











	template <typename T, typename A>
	inline auto memory_allocate(memory_t<T, A>& dest, size_t capacity) -> void
	{
		dest.ptr = dest.allocator().allocate(capacity);
	}

	template <typename T, typename A>
	inline auto memory_deallocate(memory_t<T, A>& dest) -> void
	{
		return dest.allocator().deallocate(dest.ptr, 0);
	}

	template <typename T, typename A>
	inline auto memory_construct_default(memory_t<T, A>& dest, size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			dest.allocator().construct(dest.ptr + i);
	}

	template <typename T, typename A>
	inline auto memory_construct_copy(memory_t<T, A>& dest, size_t offset, size_t count, T const& x) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			dest.allocator().construct(dest.ptr + i, x);
	}

	template <typename T, typename A>
	inline auto memory_construct_copy(memory_t<T, A>& dest, size_t offset, size_t count, T const* src) -> void
	{
		for (auto i = size_t(), j = offset; i != count; ++i, ++j)
			dest.allocator().construct(dest.ptr + j, src[i]);
	}

	template <typename T, typename A>
	inline auto memory_construct_move(memory_t<T, A>& dest, size_t offset, T&& x) -> void
	{
		dest.allocator().construct(dest.ptr + offset, std::move(x));
	}

	template <typename T, typename A>
	inline auto memory_destruct(memory_t<T, A>& dest, size_t offset, size_t count) -> void
	{
		for (auto i = offset; i != offset + count; ++i)
			dest.allocator().destroy(dest.ptr + i);
	}

	template <typename T, typename A>
	inline auto memory_memmove(memory_t<T, A>& dest, size_t dest_idx, size_t src_idx, size_t count) -> void
	{
		std::memmove(dest.ptr + dest_idx, dest.ptr + src_idx, sizeof(T) * count);
	}

	template <typename T, typename A, typename Y, typename B>
	inline auto memory_memcpy(memory_t<T, A>& dest, size_t dest_idx, memory_t<Y, B> const& src, size_t src_idx, size_t count) -> void
	{
		std::memcpy(dest.ptr + dest_idx, reinterpret_cast<T const*>(src.ptr + src_idx), sizeof(T) * count);
	}
}
