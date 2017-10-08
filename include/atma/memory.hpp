#pragma once

#include <atma/aligned_allocator.hpp>

#include <type_traits>
#include <vector>
#include <memory>


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
//      - this type doesn't handle your array bounds for you. if you allocate N
//        objects and then access objects at positions N+, you will be in random
//        memory territory, with no warning.
//
namespace atma
{
	namespace detail
	{
		template <
			typename Allocator,
			bool Empty = std::is_empty<Allocator>::value
		>
		struct base_memory_tx;


		template <typename Allocator>
		struct base_memory_tx<Allocator, false>
		{
			base_memory_tx()
			{}

			template <typename U>
			base_memory_tx(base_memory_tx<U> const& rhs)
				: allocator_(rhs.allocator())
			{}

			base_memory_tx(Allocator const& allocator)
				: allocator_(allocator)
			{}

			auto allocator() -> Allocator& { return allocator_; }
			auto allocator() const -> Allocator const& { return allocator_; }

		private:
			Allocator allocator_;
		};


		template <typename Allocator>
		struct base_memory_tx<Allocator, true>
			: protected Allocator
		{
			base_memory_tx()
			{}

			template <typename U>
			base_memory_tx(base_memory_tx<U> const& rhs)
				: Allocator(rhs.allocator())
			{}

			base_memory_tx(Allocator const& allocator)
				: Allocator(allocator)
			{}

			auto allocator() -> Allocator& { return static_cast<Allocator&>(*this); }
			auto allocator() const -> Allocator const& { return static_cast<Allocator const&>(*this); }
		};


		template <typename T, typename A>
		using base_memory_t = base_memory_tx<typename std::allocator_traits<A>::template rebind_alloc<T>>;
	}


	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct memory_t : detail::base_memory_t<T, Allocator>
	{
		using value_type = T;
		using allocator_type = Allocator;
		using reference = value_type&;
		using pointer = value_type*;

		explicit memory_t(allocator_type const& = allocator_type());
		explicit memory_t(value_type* data, allocator_type const& = allocator_type());
		explicit memory_t(size_t capacity, allocator_type const& = allocator_type());
		template <typename B> memory_t(memory_t<T, B> const&);

		auto operator = (memory_t<T, Allocator> const&) -> memory_t& = default;
		auto operator = (value_type*) -> memory_t&;
		template <typename B> auto operator = (memory_t<T, B> const&) -> memory_t&;

		operator value_type*() const { return ptr_; }
		operator value_type const*() const { return ptr_; }

		auto operator *  () const -> reference;
		auto operator [] (intptr) const -> reference;
		auto operator -> () const -> pointer;
		auto operator +  (intptr) const -> pointer;

		// allocator interface
		auto allocate(size_t) -> void;
		auto deallocate(size_t = 0) -> void;

		template <typename... Args>
		auto construct(size_t idx, Args&&...) -> void;

		template <typename... Args>
		auto construct_range(size_t idx, size_t count, Args&&...) -> void;

		// copy-construct from ranges
		auto copy_construct_range(size_t idx, value_type const* src, size_t count) -> void;
		template <typename H> auto copy_construct_range(size_t idx, H begin, H end) -> void;

		// move-construct from ranges
		auto move_construct_range(size_t idx, value_type* src, size_t count) -> void;
		template <typename H> auto move_construct_range(size_t idx, H begin, H end) -> void;

		auto destruct(size_t idx, size_t count) -> void;

		// to move memory around within the allocated area
		auto memmove(size_t dest_idx, size_t src_idx, size_t count) -> void;
		// to copy memory from somewhere else to within our allocated are
		auto memcpy(size_t idx, value_type const*, size_t count) -> void;

	private:
		using alloc_traits = std::allocator_traits<allocator_type>;

		value_type* ptr_ = nullptr;
	};




	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(A const& allocator)
		: detail::base_memory_t<T, A>(allocator)
	{}

	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(value_type* data, A const& alloc)
		: detail::base_memory_t<T, A>(alloc)
		, ptr_(data)
	{}

	template <typename T, typename A>
	inline memory_t<T, A>::memory_t(size_t capacity, A const& alloc)
		: detail::base_memory_t<T, A>(alloc)
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
		this->allocator() = rhs.allocator();
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
	inline auto memory_t<T, A>::operator -> () const -> pointer
	{
		return ptr_;
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::operator + (intptr idx) const -> pointer
	{
		return ptr_ + idx;
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::allocate(size_t count) -> void
	{
		ptr_ = alloc_traits::allocate(this->allocator(), count);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::deallocate(size_t count) -> void
	{
		alloc_traits::deallocate(this->allocator(), ptr_, count);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto memory_t<T, A>::construct(size_t idx, Args&&... args) -> void
	{
		alloc_traits::construct(this->allocator(), ptr_ + idx, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto memory_t<T, A>::construct_range(size_t idx, size_t count, Args&&... args) -> void
	{
		for (size_t i = idx; count --> 0; ++i)
			alloc_traits::construct(this->allocator(), ptr_ + i, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::copy_construct_range(size_t idx, T const* src, size_t count) -> void
	{
		for (size_t i = idx; count --> 0; ++i)
			alloc_traits::construct(this->allocator(), ptr_ + i, src[i]);
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::move_construct_range(size_t idx, T* src, size_t count) -> void
	{
		for (size_t i = idx; count --> 0; ++i)
			alloc_traits::construct(this->allocator(), ptr_ + i, std::move(src[i]));
	}

	template <typename T, typename A>
	inline auto memory_t<T, A>::destruct(size_t idx, size_t count) -> void
	{
		for (auto i = idx; i != idx + count; ++i)
			alloc_traits::destroy(this->allocator(), ptr_ + i);
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
	inline auto memory_t<T, A>::copy_construct_range(size_t idx, H begin, H end) -> void
	{
		for (size_t i = 0; begin != end; ++i, ++begin)
			alloc_traits::construct(this->allocator(), ptr_ + idx + i, *begin);
	}

	template <typename T, typename A>
	template <typename H>
	inline auto memory_t<T, A>::move_construct_range(size_t idx, H begin, H end) -> void
	{
		for (size_t i = 0; begin != end; ++i, ++begin)
			alloc_traits::construct(this->allocator(), ptr_ + idx + i, std::move(*begin));
	}




	template <typename T, typename A>
	inline auto operator + (memory_t<T,A> const& lhs, typename memory_t<T,A>::allocator_type::difference_type x) -> memory_t<T,A>
	{
		return lhs.data() + x;
	}

}


