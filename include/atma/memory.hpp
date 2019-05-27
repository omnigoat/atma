#pragma once

#include <atma/aligned_allocator.hpp>

#include <type_traits>
#include <vector>
#include <memory>

namespace atma
{
	constexpr struct memory_allocate_copy_tag {} memory_allocate_copy;
	constexpr struct memory_take_ownership_tag {} memory_take_ownership;
}


//
//  base_memory_t
//  ---------------
//
//    stores an allocator and performs Empty Base Class optimization.
//
namespace atma::detail
{
	template <typename Allocator, bool = std::is_empty_v<Allocator>>
	struct base_memory_tx;


	template <typename Allocator>
	struct base_memory_tx<Allocator, false>
	{
		using allocator_type = Allocator;
		using typename allocator_type::value_type;
		using typename allocator_type::pointer;
		using typename allocator_type::const_pointer;

		base_memory_tx()
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			: allocator_(rhs.allocator())
		{}

		base_memory_tx(Allocator const& allocator)
			: allocator_(allocator)
		{}

		auto allocator() -> allocator_type& { return allocator_; }
		auto allocator() const -> allocator_type const& { return allocator_; }

	private:
		Allocator allocator_;
	};


	template <typename Allocator>
	struct base_memory_tx<Allocator, true>
		: protected Allocator
	{
		using allocator_type = Allocator;
		using typename allocator_type::value_type;
		using typename allocator_type::pointer;
		using typename allocator_type::const_pointer;

		base_memory_tx()
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			: Allocator(rhs.allocator())
		{}

		base_memory_tx(Allocator const& allocator)
			: Allocator(allocator)
		{}

		auto allocator() -> allocator_type& { return static_cast<allocator_type&>(*this); }
		auto allocator() const -> allocator_type const& { return static_cast<allocator_type const&>(*this); }
	};


	template <typename T, typename A>
	using base_memory_t = base_memory_tx<typename std::allocator_traits<A>::template rebind_alloc<T>>;
}



namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct simple_memory_t : detail::base_memory_t<T, Allocator>
	{
		using base_type = detail::base_memory_t<T, Allocator>;
		using self_type = simple_memory_t<T, Allocator>;

		using typename base_type::allocator_type;
		using typename base_type::value_type;
		using typename base_type::pointer;
		using typename base_type::const_pointer;
		using reference = value_type&;

		explicit simple_memory_t(allocator_type const& = allocator_type());
		explicit simple_memory_t(memory_take_ownership_tag, pointer, allocator_type const& = allocator_type());
		template <typename B> simple_memory_t(simple_memory_t<T, B> const&);

		auto operator = (self_type const&) -> simple_memory_t& = default;
		auto operator = (pointer) -> simple_memory_t&;
		template <typename B> auto operator = (simple_memory_t<T, B> const&) -> simple_memory_t&;

		operator pointer() const { return ptr_; }
		operator const_pointer() const { return ptr_; }

		auto operator *  () const -> reference;
		auto operator [] (intptr) const -> reference;
		auto operator -> () const -> pointer;

	private:
		value_type* ptr_ = nullptr;
	};


	//template <typename T>
	//simple_memory_t(memory_take_ownership_tag, T* data) -> simple_memory_t<T, atma::aligned_allocator_t<T>>;

	//template <typename T, typename A>
	//simple_memory_t(memory_allocate_copy_tag, T* data, size_t, A const&) -> simple_memory_t<T, A>;
}



namespace atma
{
	template <typename T, typename A>
	inline simple_memory_t<T, A>::simple_memory_t(allocator_type const& allocator)
		: base_type(allocator)
	{}

	template <typename T, typename A>
	inline simple_memory_t<T, A>::simple_memory_t(memory_take_ownership_tag, pointer data, allocator_type const& alloc)
		: base_type(alloc)
		, ptr_(data)
	{}

	template <typename T, typename A>
	template <typename B>
	inline simple_memory_t<T, A>::simple_memory_t(simple_memory_t<T, B> const& rhs)
		: detail::base_memory_t<T, A>(rhs.allocator())
		, ptr_(rhs.ptr_)
	{}

	template <typename T, typename A>
	inline auto simple_memory_t<T, A>::operator = (pointer rhs) -> self_type&
	{
		ptr_ = rhs;
		return *this;
	}

	template <typename T, typename A>
	template <typename B>
	inline auto simple_memory_t<T, A>::operator = (simple_memory_t<T, B> const& rhs) -> self_type&
	{
		ptr_ = rhs.ptr_;
		this->allocator() = rhs.allocator();
		return *this;
	}

	template <typename T, typename A>
	inline auto simple_memory_t<T, A>::operator *  () const -> reference
	{
		return *ptr_;
	}

	template <typename T, typename A>
	inline auto simple_memory_t<T, A>::operator [] (intptr idx) const -> reference
	{
		return ptr_[idx];
	}

	template <typename T, typename A>
	inline auto simple_memory_t<T, A>::operator -> () const -> pointer
	{
		return ptr_;
	}
}


namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct operable_memory_t : public simple_memory_t<T, Allocator>
	{
		using base_type = simple_memory_t<T, Allocator>;

		using base_type::allocator_type;
		using base_type::value_type;
		using base_type::pointer;
		using base_type::const_pointer;

		// use base_type's constructor(s)
		using base_type::base_type;

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
		// to zero-clear memory
		auto memzero(size_t idx, size_t count) -> void;

	protected:
		using typename base_type::alloc_traits;
	};
}


namespace atma
{
	template <typename T, typename A>
	template <typename... Args>
	inline auto operable_memory_t<T, A>::construct(size_t idx, Args&&... args) -> void
	{
		alloc_traits::construct(this->allocator(), this->ptr_ + idx, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto operable_memory_t<T, A>::construct_range(size_t idx, size_t count, Args&&... args) -> void
	{
		for (auto px = this->ptr_ + idx; count --> 0; ++px)
			alloc_traits::construct(this->allocator(), px, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::copy_construct_range(size_t idx, value_type const* src, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx, py = src; count --> 0; ++px, ++py)
			alloc_traits::construct(this->allocator(), px, *py);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::move_construct_range(size_t idx, value_type* src, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx, py = src; count --> 0; ++px, ++py)
			alloc_traits::construct(this->allocator(), px, std::move(*py));
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::destruct(size_t idx, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx; count --> 0; ++px)
			alloc_traits::destroy(this->allocator(), px);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::memmove(size_t dest_idx, size_t src_idx, size_t count) -> void
	{
		std::memmove(this->ptr_ + dest_idx, this->ptr_ + src_idx, sizeof(value_type) * count);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::memcpy(size_t idx, value_type const* src, size_t count) -> void
	{
		std::memmove(this->ptr_ + idx, src, sizeof(value_type) * count);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::memzero(size_t idx, size_t count) -> void
	{
		std::memset(this->ptr_ + idx, 0, sizeof(value_type) * count);
	}

	template <typename T, typename A>
	template <typename H>
	inline auto operable_memory_t<T, A>::copy_construct_range(size_t idx, H begin, H end) -> void
	{
		for (auto px = this->ptr_ + idx; begin != end; ++begin, ++px)
			alloc_traits::construct(this->allocator(), px, *begin);
	}

	template <typename T, typename A>
	template <typename H>
	inline auto operable_memory_t<T, A>::move_construct_range(size_t idx, H begin, H end) -> void
	{
		for (auto px = this->ptr_ + idx; begin != end; ++begin, ++px)
			alloc_traits::construct(this->allocator(), px, std::move(*begin));
	}

	template <typename T, typename A>
	inline auto operator + (operable_memory_t<T, A> const& lhs, typename operable_memory_t<T, A>::allocator_type::difference_type x) -> typename operable_memory_t<T, A>::value_type*
	{
		return static_cast<typename operable_memory_t<T, A>::value_type*>(lhs) + x;
	}
}






namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct allocatable_memory_t : public operable_memory_t<T, Allocator>
	{
		using base_type = operable_memory_t<T, Allocator>;

		using base_type::allocator_type;
		using base_type::value_type;
		using base_type::pointer;
		using base_type::const_pointer;

		// use base_type's constructor(s)
		using base_type::base_type;

		explicit allocatable_memory_t(memory_allocate_copy_tag, const_pointer, size_t size, allocator_type const& = allocator_type());
		explicit allocatable_memory_t(size_t capacity, allocator_type const& = allocator_type());

		// allocator interface
		auto allocate(size_t) -> bool;
		auto deallocate(size_t = 0) -> void;

	private:
		using base_type::alloc_traits;
	};
}

namespace atma
{
	template <typename T, typename A>
	inline allocatable_memory_t<T, A>::allocatable_memory_t(memory_allocate_copy_tag, const_pointer data, size_t size, allocator_type const& alloc)
		: base_type(alloc)
	{
		allocate(size);
		std::memcpy(this->ptr_, data, size * sizeof(value_type));
	}

	template <typename T, typename A>
	inline allocatable_memory_t<T, A>::allocatable_memory_t(size_t capacity, allocator_type const& alloc)
		: base_type(alloc)
	{
		allocate(capacity);
	}

	template <typename T, typename A>
	inline auto allocatable_memory_t<T, A>::allocate(size_t count) -> bool
	{
		this->ptr_ = alloc_traits::allocate(this->allocator(), count);
		return this->ptr_ != nullptr;
	}

	template <typename T, typename A>
	inline auto allocatable_memory_t<T, A>::deallocate(size_t count) -> void
	{
		alloc_traits::deallocate(this->allocator(), this->ptr_, count);
	}

}

//
//  basic_memory_t
//  ----------------
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
//      - there is no size tracking of the allocation. you'll have to remember that
//        yourself, if that's important.
//
namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct basic_memory_t : protected operable_memory_t<T, Allocator>
	{
		using base_type = operable_memory_t<T, Allocator>;

		using base_type::allocator_type;
		using base_type::value_type;
		using base_type::pointer;
		using base_type::const_pointer;

		// use base_type's constructor(s)
		using base_type::base_type;

		auto memory_ops() const -> base_type& { return static_cast<base_type&>(*this); }
		auto alloc_ops() const -> allocatable_memory_t<T, Allocator>& { return static_cast<allocatable_memory_t<T, Allocator>&>(*this); }
	};


	template <typename T>
	basic_memory_t(memory_take_ownership_tag, T* data) -> basic_memory_t<T, atma::aligned_allocator_t<T>>;

	template <typename T, typename A>
	basic_memory_t(memory_allocate_copy_tag, T* data, size_t, A const&) -> basic_memory_t<T, A>;
}




