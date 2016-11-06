#pragma once

#include <atma/aligned_allocator.hpp>

#include <type_traits>
#include <vector>
#include <memory>


//
//
//
namespace atma
{
	template <typename T, typename A = aligned_allocator<T>>
	inline auto memory_construct_default(T* dest, size_t count, A& allocator = A()) -> void
	{
		for (auto v = dest; v != dest + count; ++v)
			allocator.construct(v);
	}

	template <typename T, typename A = aligned_allocator<T>>
	inline auto memory_construct_copy(T* dest, T const& x, size_t count, A& allocator = A()) -> void
	{
		for (auto v = dest; v != dest + count; ++v)
			allocator.construct(v, x);
	}

	template <typename T, typename A = aligned_allocator<T>>
	inline auto memory_construct_copy_range(T* dest, T const* src, size_t count, A& allocator = A()) -> void
	{
		for (auto v = dest; v != dest + count; ++v, ++src)
			allocator.construct(v, *src);
	}

	template <typename T, typename A = aligned_allocator<T>>
	inline auto memory_construct_move(T* dest, T&& x, A& allocator = A()) -> void
	{
		allocator.construct(*dest, std::move(x));
	}

	template <typename T, typename A = aligned_allocator<T>>
	inline auto memory_destruct(T* dest, size_t count, A& allocator = A()) -> void
	{
		for (auto v = dest; v != dest + count; ++v)
			allocator.destroy(v);
	}
}




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


	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct memory_t : detail::base_memory_t<T, Allocator>
	{
		using value_type = T;
		using reference = value_type&;

		explicit memory_t(Allocator const& = Allocator());
		explicit memory_t(value_type* data, Allocator const& = Allocator());
		explicit memory_t(size_t capacity, Allocator const& = Allocator());
		template <typename B> memory_t(memory_t<T, B> const&);

		auto operator = (value_type*) -> memory_t&;
		template <typename B> auto operator = (memory_t<T, B> const&) -> memory_t&;

		auto data() const -> value_type* { return ptr_; }

		operator value_type*() const { return ptr_; }
		operator value_type const*() const { return ptr_; }

		auto operator *  () const -> reference;
		auto operator [] (intptr) const -> reference;

		// allocator interface
		auto allocate(size_t) -> void;
		auto deallocate() -> void;

		template <typename... Args>
		auto construct(size_t idx, Args&&...) -> void;

		template <typename... Args>
		auto construct_range(size_t idx, size_t count, Args&&...) -> void;

		auto construct_copy_range(size_t idx, T const* src, size_t count) -> void;
		auto construct_move_range(size_t idx, T* src, size_t count) -> void;
		auto construct_move_range(size_t idx, memory_t& src, size_t src_idx, size_t count) -> void;

		template <typename H>
		auto construct_copy_range(size_t idx, H begin, H end) -> void;

		auto destruct(size_t idx, size_t count) -> void;

		// to move memory around within the allocated area
		auto memmove(size_t dest_idx, size_t src_idx, size_t count) -> void;
		// to copy memory from somewhere else to within our allocated are
		auto memcpy(size_t idx, T const*, size_t count) -> void;

	private:
		value_type* ptr_;
	};




	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(A const& allocator)
		: detail::base_memory_t<T, A>(allocator)
		, ptr_()
	{}

	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(value_type* data, A const& alloc)
		: detail::base_memory_t<T, A>(alloc)
		, ptr_(data)
	{}

	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(size_t capacity, A const& alloc)
		: detail::base_memory_t<T, A>(alloc)
		, ptr_()
	{
		allocate(capacity);
	}

	template <typename T, typename A>
	template <typename B>
	inline memory_t<T, A>::memory_t(memory_t<T, B> const& rhs)
		: detail::base_memory_t<T, A>(rhs.allocator())
		, ptr_(rhs.ptr_)
	{}

	template <typename T, typename A>
	inline auto memory_t<T, A>::operator = (value_type* rhs) -> memory_t&
	{
		ptr_ = rhs;
		return *this;
	}

	template <typename T, typename A>
	template <typename B>
	inline auto memory_t<T, A>::operator = (memory_t<T, B> const& rhs) -> memory_t&
	{
		ptr_ = rhs.ptr_;
		allocator() = rhs.allocator();
		return *this;
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::operator *  () const -> reference
	{
		return *ptr_;
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::operator [] (intptr idx) const -> reference
	{
		return ptr_[idx];
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::allocate(size_t capacity) -> void
	{
		ptr_ = allocator().allocate(capacity);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::deallocate() -> void
	{
		allocator().deallocate(ptr_, 0);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto memory_t<T, A>::construct(size_t idx, Args&&... args) -> void
	{
		allocator().construct(ptr_ + idx, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto memory_t<T, A>::construct_range(size_t idx, size_t count, Args&&... args) -> void
	{
		for (size_t i = 0, j = idx; i != count; ++i, ++j)
			allocator().construct(ptr_ + j, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::construct_copy_range(size_t idx, T const* src, size_t count) -> void
	{
		for (size_t i = 0, j = idx; i != count; ++i, ++j)
			allocator().construct(ptr_ + j, src[i]);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::construct_move_range(size_t idx, T* x, size_t count) -> void
	{
		for (size_t i = 0, j = idx; i != count; ++i, ++j)
			allocator().construct(ptr_ + j, std::move(x[i]));
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::construct_move_range(size_t idx, memory_t<T, A>& src, size_t src_idx, size_t count) -> void
	{
		for (auto i = size_t(), j = idx, k = src_idx; i != count; ++i, ++j, ++k)
			allocator().construct(ptr_ + j, std::move(src[k]));
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::destruct(size_t idx, size_t count) -> void
	{
		for (auto i = idx; i != idx + count; ++i)
			allocator().destroy(ptr_ + i);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::memmove(size_t dest_idx, size_t src_idx, size_t count) -> void
	{
		std::memmove(ptr_ + dest_idx, ptr_ + src_idx, sizeof(T) * count);
	}
	
	template <typename T, typename A>
	inline auto memory_t<T, A>::memcpy(size_t idx, T const* src, size_t count) -> void
	{
		std::memmove(ptr_ + idx, src, sizeof(T) * count);
	}

	template <typename T, typename A>
	template <typename H>
	inline auto memory_t<T, A>::construct_copy_range(size_t idx, H begin, H end) -> void
	{
		for (size_t i = 0; begin != end; ++i, ++begin)
			allocator().construct(ptr_ + idx + i, *begin);
	}

	template <typename T, typename A>
	inline auto operator + (memory_t<T,A> const& lhs, int64 x) -> memory_t<T,A>
	{
		return memory_t<T,A>{lhs.data() + x, lhs.allocator()};
	}





}


