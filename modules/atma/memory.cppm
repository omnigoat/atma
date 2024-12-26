module;

#include <atma/ebo_pair.hpp>
#include <atma/functor.hpp>
#include <atma/assert.hpp>

#include <atma/ranges/core.hpp>

#include <type_traits>
#include <vector>
#include <memory>

export module atma.memory;

import atma.types;
import atma.aligned_allocator;

//
// thoughts on memory things
//
//  a structure that combines an allocator & a pointer
// 
//  (basic_memory_t)
// 
//  after a lot of back-and-forth, I have come to the conclusion
//  that this structure is too low-level for any reasonable
//  abstraction. the moment we start adding operators etc., we
//  begin to approach the more complex structures that were
//  using this structure internally
//
//  some basic operators are provided to prevent the loss of
//  the allocator information
//



// forward-declares
export namespace atma
{
	struct dest_memory_tag_t;
	struct src_memory_tag_t;
	constexpr size_t unbounded_memory_size = ~size_t();

	constexpr struct allocate_n_tag {} allocate_n;

	template <typename Tag, typename T, typename A = std::allocator<T>>
	struct memxfer_t;
}


//
//  base_memory_t
//  ---------------
//
//    stores an allocator and performs Empty Base Class optimization.
//
export namespace atma::detail
{
	template <typename Allocator, bool = std::is_empty_v<Allocator>>
	struct base_memory_impl;

	template <typename Allocator>
	struct base_memory_impl<Allocator, false>
	{
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		base_memory_impl() = default;
		base_memory_impl(base_memory_impl const&) = default;

		base_memory_impl(Allocator const& allocator)
			noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
			: allocator_(allocator)
		{}

		template <typename U>
		base_memory_impl(base_memory_impl<U> const& rhs)
			noexcept(noexcept(Allocator(rhs.get_allocator())))
			: allocator_(rhs.get_allocator())
		{}

		auto get_allocator() const -> allocator_type { return allocator_; }

	private:
		Allocator allocator_;
	};

	template <typename Allocator>
	struct base_memory_impl<Allocator, true>
		: protected Allocator
	{
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		base_memory_impl() = default;
		base_memory_impl(base_memory_impl const&) = default;

		base_memory_impl(Allocator const& allocator)
			: Allocator(allocator)
		{}

		template <typename U>
		base_memory_impl(base_memory_impl<U> const& rhs)
			noexcept(noexcept(Allocator(rhs.get_allocator())))
			: Allocator(rhs.get_allocator())
		{}

		auto get_allocator() const -> allocator_type { return static_cast<allocator_type const&>(*this); }
	};

	template <typename T, typename A>
	using base_memory_t = base_memory_impl<typename std::allocator_traits<A>::template rebind_alloc<T>>;
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
export namespace atma
{
	template <typename T, typename Allocator = std::allocator<T>>
	struct basic_memory_t : detail::base_memory_t<T, Allocator>
	{
		using base_type = detail::base_memory_t<T, Allocator>;

		using allocator_type   = typename base_type::allocator_type;
		using allocator_traits = typename base_type::allocator_traits;
		using value_type       = typename allocator_traits::value_type;
		using pointer          = typename allocator_traits::pointer;
		using const_pointer    = typename allocator_traits::const_pointer;
		using reference        = typename allocator_traits::value_type&;

		// inherit constructors
		using base_type::base_type;

		constexpr basic_memory_t() = default;

		basic_memory_t(basic_memory_t const&)
		requires std::allocator_traits<Allocator>::is_always_equal::value
			= default;
		
		explicit basic_memory_t(pointer, allocator_type const& = allocator_type())
			noexcept(std::is_nothrow_copy_constructible_v<allocator_type>);

		// allocator interface
		auto self_allocate(size_t) -> bool;
		auto self_deallocate(size_t = 0) -> void;

		// "data" interface
		auto data() const { return ptr; }
		operator pointer() const { return ptr; }

		pointer ptr = nullptr;
	};


	// deduction guides
	template <typename T>
	basic_memory_t(T*) -> basic_memory_t<T>;

	template <typename T, typename A>
	basic_memory_t(T*, A const&) -> basic_memory_t<T, A>;


	// comparison
	template <typename T, typename A>
	inline auto operator == (basic_memory_t<T, A> const& lhs, basic_memory_t<T, A> const& rhs)
	{
		return lhs.ptr == rhs.ptr;
	}

	// addition
	template <typename T, typename A>
	inline auto operator + (basic_memory_t<T, A> const& lhs, std::integral auto rhs)
	{
		return basic_memory_t<T, A>{lhs.ptr + rhs, lhs.get_allocator()};
	}

	// std::ostream
	template <typename T, typename A>
	inline decltype(auto) operator << (std::ostream& stream, basic_memory_t<T, A> const& x)
	{
		return stream << "basic_memory_t{0x" << x.ptr << "}";
	}
}

export namespace atma
{
	using memory_t = basic_memory_t<byte>;
}



//
//  unique_memory_t
//  -----------------
//
//
//
export namespace atma
{
	template <typename T, typename Alloc>
	struct basic_unique_memory_t
	{
		using value_type = T;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using allocator_type = Alloc;
		using backing_t = atma::basic_memory_t<std::remove_cv_t<T>, allocator_type>;

		basic_unique_memory_t() noexcept = default;
		basic_unique_memory_t(basic_unique_memory_t const&) = delete;
		basic_unique_memory_t(basic_unique_memory_t&&);
		~basic_unique_memory_t();

		// converting constructors
		explicit basic_unique_memory_t(allocator_type const& alloc)
			noexcept(noexcept(backing_t(alloc)));

		explicit basic_unique_memory_t(pointer data, size_t size, allocator_type const& alloc = allocator_type())
			noexcept(noexcept(backing_t(data, alloc)));

		// named constructors
		explicit basic_unique_memory_t(allocate_n_tag, size_t size, allocator_type const& alloc = allocator_type());


		auto operator = (basic_unique_memory_t const&) -> basic_unique_memory_t & = delete;
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
		//static constexpr struct unique_memory_allocate_copy_tag copy_tag;
		//static constexpr struct unique_memory_take_ownership_tag own_tag;
		//
		//basic_unique_memory_t(unique_memory_allocate_copy_tag, void const* data, size_t size_bytes);
		//basic_unique_memory_t(unique_memory_take_ownership_tag, void* data, size_t size_bytes);

	private:
		backing_t memory_;
		size_t size_ = 0;

		template <typename, typename> friend struct basic_unique_memory_t;
	};
}

export namespace atma
{
	using unique_memory_t = basic_unique_memory_t<byte, atma::aligned_allocator_t<byte, 4>>;

	template <typename T>
	using typed_unique_memory_t = basic_unique_memory_t<T, atma::aligned_allocator_t<byte, 4>>;
}






//
// basic_memory_t implementation
//
namespace atma
{
	template <typename T, typename A>
	inline basic_memory_t<T, A>::basic_memory_t(pointer data, allocator_type const& alloc)
		noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
		: base_type(alloc)
		, ptr(data)
	{}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::self_allocate(size_t size) -> bool
	{
		auto allocator = this->get_allocator();
		this->ptr = allocator_traits::allocate(allocator, size);
		return this->ptr != nullptr;
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::self_deallocate(size_t size) -> void
	{
		auto allocator = this->get_allocator();
		allocator_traits::deallocate(allocator, this->ptr, size);
	}
}




//
// basic_unique_memory_t implementation
//
namespace atma
{
	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(allocator_type const& alloc)
		noexcept(noexcept(backing_t(alloc)))
		: memory_(alloc)
	{}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(basic_unique_memory_t&& rhs)
		: memory_{rhs.memory_}
		, size_{rhs.size_}
	{
		rhs.memory_.ptr = nullptr;
		rhs.size_ = 0;
	}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::~basic_unique_memory_t()
	{
		memory_.self_deallocate(size_);
	}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(pointer data, size_t size, allocator_type const& alloc)
		noexcept(noexcept(backing_t(data, alloc)))
		: memory_(data, alloc)
		, size_(size)
	{}

	template <typename T, typename A>
	inline basic_unique_memory_t<T, A>::basic_unique_memory_t(allocate_n_tag, size_t size, allocator_type const& alloc)
		: memory_(alloc)
		, size_(size)
	{
		if (size_ > 0)
		{
			memory_.self_allocate(size_);
		}
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
		return memory_.ptr;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::end() const -> value_type const*
	{
		return memory_.ptr + size_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::begin() -> value_type*
	{
		return memory_.ptr;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::end() -> value_type*
	{
		return memory_.ptr + size_;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::reset(void* mem, size_t size) -> void
	{
		memory_.self_deallocate(size_);
		memory_.self_allocate(size);
		memory_.memcpy(0, mem, size);
		size_ = size;
	}

	template <typename T, typename A>
	inline auto basic_unique_memory_t<T, A>::reset(size_t size) -> void
	{
		if (size != size_)
		{
			memory_.self_deallocate(size_);
			memory_.self_allocate(size);
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
		memory_.ptr = nullptr;
		return tmp;
	}
}



export namespace atma
{
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








// get_allocator
export namespace atma
{
	inline constexpr auto get_allocator = functor_cascade_t
	{
		[](auto&& r) -> decltype(r.get_allocator()) { return r.get_allocator(); },
		[](auto&& r) { return std::allocator<rm_cvref_t<value_type_of_t<decltype(r)>>>(); }
	};
}

// concept: memory
export namespace atma
{
	template <typename Memory>
	concept memory_concept = requires(Memory memory)
	{
		typename rm_ref_t<Memory>::value_type;
		typename rm_ref_t<Memory>::allocator_type;
		{ std::data(memory) } -> std::same_as<std::add_pointer_t<typename rm_ref_t<Memory>::value_type>>;
		{ atma::get_allocator(memory) };
	};

	template <typename Memory>
	concept bounded_memory_concept = memory_concept<Memory> && requires(Memory memory)
	{
		{ std::size(memory) };
	};

	template <typename Memory>
	concept dest_tagged_concept = std::is_same_v<dest_memory_tag_t, typename rm_ref_t<Memory>::tag_type>;

	template <typename Memory>
	concept src_tagged_concept = std::is_same_v<src_memory_tag_t, typename rm_ref_t<Memory>::tag_type>;

	// these are the primary concepts used throughout the memory system to
	// for function arguments, to determine behaviour/optimziations we can
	// infer -- most notably asserting sizes match
	template <typename Memory> concept dest_memory_concept = memory_concept<Memory> && dest_tagged_concept<Memory>;
	template <typename Memory> concept src_memory_concept = memory_concept<Memory> && src_tagged_concept<Memory>;
	template <typename Memory> concept dest_bounded_memory_concept = bounded_memory_concept<Memory> && dest_tagged_concept<Memory>;
	template <typename Memory> concept src_bounded_memory_concept = bounded_memory_concept<Memory> && src_tagged_concept<Memory>;
}

// memory traits
export namespace atma
{
	template <memory_concept Memory>
	using memory_value_type_t = typename rm_ref_t<Memory>::value_type;
}


//
// memxfer_t
// -----------------
//   a type used for transferring memory around.
//
//   this structure contains a pointer to some memory and possibly a
//   stateful allocator. stateless allocators add no additional size to
//   this type, so it's usually just the size of a pointer. stateful
//   allocators will make the size of this type grow.
//
namespace atma
{
	template <typename Tag, typename T, typename A>
	struct memxfer_t
	{
		using value_type = T;
		using allocator_type = A;
		using tag_type = Tag;

		constexpr memxfer_t(memxfer_t const&) = default;
		constexpr ~memxfer_t() = default;

		// default-constructor only allowed if allocator doesn't hold state
		constexpr explicit memxfer_t()
		requires std::is_empty_v<allocator_type>
			: alloc_and_ptr_(allocator_type(), nullptr)
		{}

		// so this is written real fucking weird because it needs to be templated.
		// if it is not templated, then there are two available overloads for a
		// statically-sized array: the one in (derived type) bounded_memxfer_t, and
		// this one, which decays the static-array to a pointer.
		//
		// with the two possible overloads, C++ tries to find the best match for the
		// argument-types. for some fucking strange reason, C++ will prefer non-
		// templated methods over templated ones. if the argument is a static-array,
		// and our two methods look like this...
		// 
		//   memxfer_t(T* ptr) { ... }
		// 
		//   template <size_t N>
		//   bounded_memxfer_t(T(&arr)[N]) { ... }
		// 
		// ... then c++ will prefer the one in the base class, and DECAY THE ARRAY.
		// 
		// how dumb is that? the method that takes a statically-sized array is
		// obviously a better fit, regardless of if it's templated. sigh.
		//
		template <typename Y>
		constexpr memxfer_t(Y ptr)
		requires std::is_empty_v<allocator_type> && std::is_same_v<Y, T*>
			: alloc_and_ptr_(allocator_type(), ptr)
		{}

		constexpr memxfer_t(allocator_type a, T* ptr)
			: alloc_and_ptr_(a, ptr)
		{}

		// we, a range of const-elements, can always adapt a range of non-const elements
		constexpr memxfer_t(memxfer_t<Tag, std::remove_const_t<T>, A> const& rhs)
			requires std::is_const_v<T>
			: alloc_and_ptr_(rhs.get_allocator(), (T const*)rhs.data())
		{}

		memxfer_t& operator = (memxfer_t const& rhs)
		{
			alloc_and_ptr_ = rhs.alloc_and_ptr_;
			return *this;
		}

		allocator_type& get_allocator() const { return const_cast<memxfer_t*>(this)->alloc_and_ptr_.first(); }

		auto data() -> value_type* { return alloc_and_ptr_.second(); }
		auto data() const -> value_type const* { return alloc_and_ptr_.second(); }

		auto operator [](size_t idx) const -> value_type&
		{
			return data()[idx];
		}

	protected:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T*>;
		alloc_and_ptr_type alloc_and_ptr_;
	};
}

namespace atma::detail
{
	template <size_t Extent, size_t Offset, size_t Count>
	struct subextent_t
	{
		static_assert(Count == std::dynamic_extent || Offset != std::dynamic_extent,
			"if count is a Real Extent, then our offset can't be dynamic_extent");
		
		static constexpr size_t value = Count != std::dynamic_extent ? Count
			: Extent != std::dynamic_extent ? (Extent - Offset)
			: std::dynamic_extent;
	};

	template <size_t Extent, size_t Offset, size_t Count>
	inline constexpr size_t subextent_v = subextent_t<Extent, Offset, Count>::value;

	inline size_t subextent(size_t extent, size_t offset, size_t count)
	{
		return count != std::dynamic_extent ? count
			: extent != std::dynamic_extent ? (extent - offset)
			: std::dynamic_extent;
	}
}

//
// bounded_memxfer_t
// --------------------------
//   a type used for transferring memory around.
//
//   this structure is just a memxfer_t but with a size. this allows 
//   it to have empty(), size(), begin(), and end() methods. this allows
//   us options for optimization in some algorithms
//
namespace atma::detail
{
	// static-extent version
	//----------------------------------
	template <typename Tag, typename T, size_t Extent, typename A>
	struct bounded_memxfer_impl_t
		: memxfer_t<Tag, T, A>
	{
		using base_type = memxfer_t<Tag, T, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;
		using tag_type = Tag;

		static constexpr size_t extent_v = Extent;

		// inherit constructors
		using base_type::base_type;

		// observers
		auto empty() const -> bool { return extent_v == 0; }
		auto size() const -> size_t { return extent_v; }
		auto size_bytes() const -> size_t { return extent_v * sizeof(value_type); }
	};

	// dynamic-extent version
	//----------------------------------
	template <typename Tag, typename T, typename A>
	struct bounded_memxfer_impl_t<Tag, T, std::dynamic_extent, A>
		: memxfer_t<Tag, T, A>
	{
		using base_type = memxfer_t<Tag, T, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;
		using tag_type = Tag;

		static constexpr size_t extent_v = std::dynamic_extent;

		constexpr bounded_memxfer_impl_t(bounded_memxfer_impl_t const& rhs)
			: base_type(rhs)
			, size_(rhs.size())
		{}

		// adapt a range with a static extent
		template <size_t RhsExtent>
		constexpr bounded_memxfer_impl_t(bounded_memxfer_impl_t<Tag, T, RhsExtent, A> const& rhs)
			requires !std::is_const_v<T>
			: base_type(rhs)
			, size_(rhs.size())
		{}

		// adapt a non-const range (if we're a const-range)
		template <size_t RhsExtent>
		constexpr bounded_memxfer_impl_t(bounded_memxfer_impl_t<Tag, std::remove_const_t<T>, RhsExtent, A> const& rhs)
			requires std::is_const_v<T>
			: base_type(rhs.get_allocator(), (T*)rhs.data())
			, size_(rhs.size())
		{}

		constexpr bounded_memxfer_impl_t(allocator_type allocator, T* ptr, size_t size)
			: base_type(allocator, ptr)
			, size_(size)
		{}

		constexpr bounded_memxfer_impl_t(T* ptr, size_t size)
			requires std::is_empty_v<allocator_type>
			: base_type(allocator_type(), ptr)
			, size_(size)
		{}

		bounded_memxfer_impl_t& operator = (bounded_memxfer_impl_t const& rhs)
		{
			return (bounded_memxfer_impl_t&)base_type::operator = (rhs);
		}

		// observers
		constexpr auto empty() const -> bool { return size_ == 0; }
		constexpr auto size() const -> size_t { return size_; }
		constexpr auto size_bytes() const -> size_t { return size_ * sizeof(value_type); }

	private:
		// explicitly hide memxfer constructors because we must initialize size
		using base_type::base_type;

	protected:
		size_t size_ = std::dynamic_extent;
	};
}

export namespace atma
{
	template <typename Tag, typename T, size_t Extent, typename A>
	struct bounded_memxfer_t
		: detail::bounded_memxfer_impl_t<Tag, T, Extent, A>
	{
		using base_type = detail::bounded_memxfer_impl_t<Tag, T, Extent, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;
		using tag_type = Tag;

		template <size_t Offset, size_t Count>
		using subspan_type = bounded_memxfer_t<Tag, T, detail::subextent_v<Extent, Offset, Count>, allocator_type>;
		using dynamic_subspan_type = bounded_memxfer_t<Tag, T, std::dynamic_extent, A>;

		using base_type::extent_v;

		// inherit constructors
		using base_type::base_type;

		// constructors
		template <size_t N>
		constexpr bounded_memxfer_t(T(&arra)[N])
		requires std::is_empty_v<allocator_type>
			: base_type(allocator_type(), static_cast<T*>(arra), N)
		{}

		// assignment operators
		bounded_memxfer_t& operator = (bounded_memxfer_t const& rhs)
		{
			this->size_ = rhs.size_;
			return (bounded_memxfer_t&)base_type::operator = (rhs);
		}

		// element access
		auto begin()       -> value_type* { return this->alloc_and_ptr_.second(); }
		auto end()         -> value_type* { return this->alloc_and_ptr_.second() + this->size(); }
		auto begin() const -> value_type const* { return this->alloc_and_ptr_.second(); }
		auto end() const   -> value_type const* { return this->alloc_and_ptr_.second() + this->size(); }

		// compile-time subviews
		template <size_t Offset, size_t Count = std::dynamic_extent>
		constexpr auto subspan() const -> subspan_type<Offset, Count>
		{
			if constexpr (detail::subextent_v<Extent, Offset, Count> == std::dynamic_extent)
			{
				return {this->get_allocator(), this->data() + Offset, detail::subextent(this->size(), Offset, Count)};
			}
			else
			{
				return {this->get_allocator(), this->data() + Offset};
			}
		}

		template <size_t Begin, size_t End>
		constexpr auto subspan_bounds() const -> bounded_memxfer_t<Tag, T, (End - Begin), allocator_type>
		{
			return {this->get_allocator(), this->data() + Begin};
		}

		template <size_t N> constexpr auto skip() const { return this->subspan<N>(); }
		template <size_t N> constexpr auto take() const { return this->subspan<0, N>(); }
		
		template <size_t N>
		requires (extent_v != std::dynamic_extent)
		constexpr auto last() const { return this->subspan<extent_v - N>(); }

		// run-time subviews
		constexpr auto subspan(size_t offset, size_t count = std::dynamic_extent) const -> dynamic_subspan_type
		{
			return {this->get_allocator(), (T*)this->data() + offset, detail::subextent(this->size(), offset, count)};
		}

		constexpr auto subspan_bounds(size_t begin, size_t end) const -> bounded_memxfer_t<Tag, T, std::dynamic_extent, allocator_type>
		{
			return {this->get_allocator(), this->data() + begin, (end - begin)};
		}

		constexpr auto front() const { return this->operator [](0); }
		constexpr auto back() const { return this->operator [](this->size() - 1); }

		// length-based views
		constexpr auto skip(size_t n) const { return this->subspan(n); }
		constexpr auto take(size_t n) const { return this->subspan(0, n); }
		constexpr auto last(size_t n) const { return this->subspan(this->size() - n); }
		constexpr auto drop(size_t n) const { return this->subspan(0, this->size() - n); }

		// index-based views
		constexpr auto from(size_t n) const { return this->subspan(n); }
		constexpr auto to(size_t n) const { return this->subspan(0, n); }
		constexpr auto from_to(size_t idx_from, size_t idx_to) const { return this->subspan(idx_from, idx_to - idx_from); }

		//
		constexpr auto split(size_t n) const { return std::make_tuple(this->subspan(0, n), this->subspan(n)); }
	};
}


//
// dest_/src_ versions of memory-ranges
// --------------------------
//
//   SERIOUSLY write some docos
//
export namespace atma
{
	// dest_memxfer_t
	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using dest_memxfer_t = memxfer_t<dest_memory_tag_t, T, A>;

	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using dest_bounded_memxfer_t = bounded_memxfer_t<dest_memory_tag_t, T, std::dynamic_extent, A>;

	// src_memxfer_t
	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using src_memxfer_t = memxfer_t<src_memory_tag_t, T, A>;

	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using src_bounded_memxfer_t = bounded_memxfer_t<src_memory_tag_t, T, std::dynamic_extent, A>;
}



//
//
// xfer_dest / xfer_src
// ----------------------
//  implementations of how to make dest/src ranges from various arguments
//
//
namespace atma::detail
{
	// memxfer_range_of
	template <typename Tag, typename Range>
	using memxfer_range_of_t = memxfer_t<Tag, value_type_of_t<Range>, allocator_type_of_t<Range>>;

	template <typename Tag, typename Range>
	using bounded_memxfer_range_of_t = bounded_memxfer_t<Tag, value_type_of_t<Range>, std::dynamic_extent, allocator_type_of_t<Range>>;

	template <typename T>
	using nonconst_std_allocator = std::allocator<std::remove_const_t<T>>;

	template <typename tag_type>
	struct xfer_make_from_ptr_
	{
		template <typename T>
		auto operator()(T* data) const
		{
			return memxfer_t<tag_type, T, nonconst_std_allocator<T>>(data);
		}

		template <typename T, typename A>
		auto operator()(A&& allocator, T* data) const
		{
			return memxfer_t<tag_type, T, rm_ref_t<A>>(std::forward<A>(allocator), data);
		}

		template <typename T>
		auto operator()(T* data, size_t sz) const
		{
			return bounded_memxfer_t<tag_type, T, std::dynamic_extent, nonconst_std_allocator<T>>(data, sz);
		}

		template <typename T, typename A>
		auto operator()(A&& allocator, T* data, size_t sz) const
		{
			return bounded_memxfer_t<tag_type, T, std::dynamic_extent, rm_ref_t<A>>(std::forward<A>(allocator), data, sz);
		}

		template <typename T, size_t N>
		auto operator()(T (&data)[N]) const
		{
			return bounded_memxfer_t<tag_type, T, nonconst_std_allocator<T>>((T*)data, N);
		}
	};

	template <typename tag_type>
	struct xfer_make_from_sized_contiguous_range_
	{
		template <sized_and_contiguous_range R>
		auto operator ()(R&& range) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<R>>
		{
			return {get_allocator(range), std::ranges::data(range), std::ranges::size(range)};
		};
	};

	template <typename tag_type>
	struct xfer_make_from_contiguous_range_
	{
		template <std::ranges::contiguous_range R>
		auto operator ()(R&& range) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<R>>
		{
			auto const sz = std::distance(std::begin(range), std::end(range));

			return {get_allocator(range), std::addressof(*std::begin(range)), sz};
		}

		template <std::ranges::contiguous_range R>
		auto operator ()(R&& range, size_t size) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<R>>
		{
			return {get_allocator(range), std::addressof(*std::begin(range)), size};
		}

		template <std::ranges::contiguous_range R>
		auto operator ()(R&& range, size_t offset, size_t size) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<R>>
		{
			return {get_allocator(range), std::addressof(*std::begin(range)) + offset, size};
		}
	};

	template <typename tag_type>
	struct xfer_make_from_memory_
	{
		template <memory_concept M>
		auto operator ()(M&& memory) const -> memxfer_range_of_t<tag_type, rm_ref_t<M>>
		{
			return {get_allocator(memory), std::data(memory)};
		}

		template <memory_concept M>
		auto operator ()(M&& memory, size_t size) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<M>>
		{
			return {get_allocator(memory), std::data(memory), size};
		}

		template <memory_concept M>
		auto operator ()(M&& memory, size_t offset, size_t size) const -> bounded_memxfer_range_of_t<tag_type, rm_ref_t<M>>
		{
			return {get_allocator(memory), std::data(memory) + offset, size};
		}
	};

	template <typename Tag>
	struct xfer_make_from_iterator_pair_
	{
		template <std::contiguous_iterator It>
		auto operator ()(It begin, It end) const
			-> bounded_memxfer_t
			< Tag
			, std::remove_reference_t<decltype(*std::declval<It>())>
			, std::dynamic_extent
			, std::allocator<std::iter_value_t<It>>>
		{
			size_t const sz = std::distance(begin, end);

			return {std::addressof(*begin), sz};
		}
	};
}

namespace atma::detail
{
	template <typename tag_type>
	using xfer_range_maker_ = functor_cascade_t
	<
		// first match against pointer
		xfer_make_from_ptr_<tag_type>,

		// then match anything that satisfies the contiguous-range concept AND
		// satisfies the sized-range concept (std::begin/end & std::size)
		xfer_make_from_sized_contiguous_range_<tag_type>,

		// then try _only_ the contiguous-range concept (use std::distance instead of std::size)
		xfer_make_from_contiguous_range_<tag_type>,

		// then try anything that satisfies the memory concept (get_allocator & std::data)
		xfer_make_from_memory_<tag_type>,

		// iterator-pair version
		xfer_make_from_iterator_pair_<tag_type>
	>;
}

export namespace atma
{
	constexpr auto xfer_dest = detail::xfer_range_maker_<dest_memory_tag_t>();
	constexpr auto xfer_src  = detail::xfer_range_maker_<src_memory_tag_t>();
}

// shortcut for half-open ranges ??
export namespace atma
{
	template <typename T>
	inline auto xfer_dest_between(T* ptr, size_t from, size_t to)
	{
		return xfer_dest(ptr, from, to - from);
	}

	template <typename T>
	inline auto xfer_src_between(T* ptr, size_t from, size_t to)
	{
		return xfer_src(ptr, from, to - from);
	}

	template <std::ranges::contiguous_range Range>
	inline auto xfer_dest_between(Range&& range, size_t from, size_t to)
	{
		return xfer_dest(std::forward<Range>(range), from, to - from);
	}

	template <std::ranges::contiguous_range Range>
	inline auto xfer_src_between(Range&& range, size_t from, size_t to)
	{
		return xfer_src(std::forward<Range>(range), from, to - from);
	}


	template <typename T>
	inline auto xfer_src_list(std::initializer_list<T> things) -> src_bounded_memxfer_t<T>
	{
		return {std::allocator<T>(), std::data(things), std::size(things)};
	}
}





export namespace atma::detail
{
	template <typename R>
	constexpr auto allocator_type_of_range_(R&& a) -> std::allocator<rm_cvref_t<decltype(*std::begin(a))>>;

	template <typename A>
	constexpr auto allocator_traits_of_allocator_(A&& a) -> std::allocator_traits<rm_ref_t<A>>;

	template <typename A>
	using allocator_traits_of_t = std::allocator_traits<rm_ref_t<A>>;
}






//##################################
//
//  begin algorithms here
//
//##################################


//
// detail::_memory_range_delegate_
// ----------------------------------
// 
// this delegate is used for all memory-routines that operate on two ranges
//
export namespace atma::detail
{
	template <typename F>
	inline constexpr auto _memory_range_delegate_ = functor_cascade_t
	{
		functor_cascade_fwds_t<F>(),

		[](auto const& operation, dest_memory_concept auto&& dest, src_memory_concept auto&& src, size_t sz)
		{
			constexpr bool dest_is_bounded = bounded_memory_concept<decltype(dest)>;
			constexpr bool src_is_bounded = bounded_memory_concept<decltype(src)>;

			ATMA_ASSERT(sz != unbounded_memory_size);

			if constexpr (dest_is_bounded && src_is_bounded)
				ATMA_ASSERT(dest.size() == src.size());

			if constexpr (dest_is_bounded)
				ATMA_ASSERT(dest.size() == sz);

			if constexpr (src_is_bounded)
				ATMA_ASSERT(src.size() == sz);

			operation(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto const& operation, dest_bounded_memory_concept auto&& dest, src_bounded_memory_concept auto&& src)
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			operation(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto const& operation, dest_bounded_memory_concept auto&& dest, src_memory_concept auto&& src)
		{
			operation(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto const& operation, dest_memory_concept auto&& dest, src_bounded_memory_concept auto&& src)
		{
			operation(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(src));
		},

		// this method is the odd one out, taking a pair of iterators as the source
		//
		// not all operations are guaranteed to support this, but should provide an
		// informative error-message to that effect
		[](auto const& operation, dest_memory_concept auto&& dest, std::input_iterator auto begin, auto end)
		requires std::equality_comparable_with<decltype(begin), decltype(end)>
		{
			operation(
				get_allocator(dest),
				std::data(dest),
				begin, end);
		}
	};
}


namespace atma::detail
{
	template <typename Allocator>
	inline void construct_with_allocator_traits_(Allocator&& allocator, auto&&... args)
	{
		using allocator_traits = allocator_traits_of_t<decltype(allocator)>;
		allocator_traits::construct(std::forward<Allocator>(allocator), std::forward<decltype(args)>(args)...);
	}
}

// memory_construct_at
export namespace atma
{
	constexpr auto memory_construct_at = [](memory_concept auto&& dest, auto&&... args)
	{
		auto&& allocator = get_allocator(dest);
		detail::construct_with_allocator_traits_(allocator, std::data(dest), std::forward<decltype(args)>(args)...);
	};
}


// memory_default_construct / memory_value_construct / memory_direct_construct
namespace atma::detail
{
	constexpr auto _memory_default_construct_ = [](dest_memory_concept auto&& dest, size_t sz)
	{
		auto px = std::data(dest);

		for (size_t i = 0; i != sz; ++i, ++px)
		{
			// allocator is not capable of default-constructing
			::new (px) memory_value_type_t<decltype(dest)>;
		}
	};

	constexpr auto _memory_value_construct_ = [](dest_memory_concept auto&& dest, size_t sz)
	{
		auto&& allocator = get_allocator(dest);
		auto* px = std::data(dest);

		for (size_t i = 0; i != sz; ++i, ++px)
		{
			construct_with_allocator_traits_(allocator, px);
		}
	};

	constexpr auto _memory_direct_construct_ = [](dest_memory_concept auto&& dest, size_t sz, auto&&... args)
	{
		auto&& allocator = get_allocator(dest);
		auto* px = std::data(dest);

		for (size_t i = 0; i != sz; ++i, ++px)
		{
			construct_with_allocator_traits_(allocator, px, std::forward<decltype(args)>(args)...);
		}
	};

	template <typename F>
	constexpr auto _memory_range_construct_delegate_ = functor_cascade_t
	{
		functor_cascade_fwds_t<F>{},

		[](auto& f, dest_bounded_memory_concept auto&& dest)
		{
			f(dest, std::size(dest));
		},

		[](auto& f, dest_memory_concept auto&& dest, size_t sz)
		{
			f(dest, sz);
		}
	};
}

export namespace atma
{
	inline constexpr auto memory_default_construct = detail::_memory_range_construct_delegate_<decltype(detail::_memory_default_construct_)>;
	inline constexpr auto memory_value_construct = detail::_memory_range_construct_delegate_<decltype(detail::_memory_value_construct_)>;

	inline constexpr auto memory_direct_construct = [](dest_memory_concept auto&& dest, auto&&... args)
	{
		detail::_memory_direct_construct_(
			dest,
			std::size(dest),
			std::forward<decltype(args)>(args)...);
	};
}


// memory_copy_construct / memory_move_construct
export namespace atma::detail
{
	inline constexpr auto _memory_copy_construct_ = functor_cascade_t
	{
		[](auto&& dest_allocator, auto&&, auto* px, auto* py, size_t size)
		{
			for (size_t i = 0; i != size; ++i, ++px, ++py)
			{
				construct_with_allocator_traits_(dest_allocator, px, *py);
			}
		},

		[](auto&& allocator, auto* px, auto begin, auto end)
		{
			for (; begin != end; ++px, ++begin)
			{
				construct_with_allocator_traits_(allocator, px, *begin);
			}
		}
	};

	inline constexpr auto _memory_move_construct_ = functor_cascade_t
	{
		[](auto&& dest_allocator, auto&&, auto* px, auto* py, size_t size)
		{
			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != size; ++i, ++px, ++py)
				{
					construct_with_allocator_traits_(dest_allocator, px, std::move(*py));
				}
			}
			// src range earlier, move backwards
			else
			{
				auto pxe = px + size;
				auto pye = py + size;
				for (size_t i = size; i != 0; --i)
				{
					--pxe, --pye;
					construct_with_allocator_traits_(dest_allocator, pxe, std::move(*pye));
				}
			}
		},

		[](auto&& allocator, auto* px, auto begin, auto end)
		{
			for ( ; begin != end; ++px, ++begin)
			{
				construct_with_allocator_traits_(allocator, px, std::move(*begin));
			}
		}
	};
}





// destruct
export namespace atma
{
	template <memory_concept Memory>
	inline auto memory_destruct_at(Memory&& dest) -> void
	{
		auto&& allocator = get_allocator(dest);
		using allocator_traits = detail::allocator_traits_of_t<decltype(allocator)>;
		allocator_traits::destroy(allocator, std::data(dest));
	}

	template <typename DT, typename DA>
	inline auto memory_destruct(dest_bounded_memxfer_t<DT, DA> dest_range) -> void
	{
		for (auto& x : dest_range)
			std::allocator_traits<DA>::destroy(dest_range.get_allocator(), &x);
	}
}

//
// memory_copy
// -------------
//
export namespace atma::detail
{
	inline constexpr auto _memory_copy_ = functor_cascade_t
	{
		[] (auto&&, auto&&, auto* px, auto* py, size_t size)
		requires !std::is_trivial_v<std::remove_reference_t<decltype(*px)>>
		{
			static_assert(actually_false_v<decltype(*px)>,
				"calling memory-copy (so ultimately ::memcpy) on a non-trivial type");
		},

		[](auto&&, auto&&, auto* px, auto* py, size_t size)
		{
			size_t const size_bytes = size * sizeof(*px);
			::memcpy(px, py, size_bytes);
		}
	};
}


//
// memory_move
// -------------
// 
// a type-aware version of memmove
//
export namespace atma::detail
{
	inline constexpr auto _memory_move_ = functor_cascade_t
	{
		[]<typename T>(auto&&, auto&&, T* px, T const* py, size_t size)
		requires !std::is_trivial_v<std::remove_reference_t<T>>
			{ static_assert(actually_false_v<T>,
				"calling memory-move (so ultimately ::memmove) on a non-trivial type"); },

		[]<typename T>(auto&& dest_allocator, auto&& src_allocator, T* px, T const* py, size_t size)
		{
			size_t const size_bytes = size * sizeof(T);
			::memmove(px, py, size_bytes);
		}
	};
}


// memory_fill
export namespace atma
{
	template <typename DT, typename DA, typename V>
	inline auto memory_fill(dest_bounded_memxfer_t<DT, DA> dest_range, V v) -> void
	{
		std::fill_n(dest_range.begin(), dest_range.size(), v);
	}
}



//
// memory_relocate
// -----------------
// 
// a higher-level, type-aware routine that tries to move a contiguous
// range of elements from one position in memory to another. allows
// overlapping ranges. will attempt the operation in the following order,
// performing the first possible (note: destructs the source element
// unless it was trivial):
// 
//   1. memmove, if the type is trivial
//   2. move-construct
//   3. copy-construct
//   4. default-construct & move-assign
//   5. default-construct & copy-assign
//   ~. nada, we're out of options
//
export namespace atma::detail
{
	template <typename T>
	concept trivially_copyable = std::is_trivially_copyable_v<T>;

	template <typename T>
	concept move_constructible = std::is_move_constructible_v<T>;

	template <typename T>
	concept copy_constructible = std::is_copy_constructible_v<T>;

	template <typename T>
	concept move_assignable = std::is_move_assignable_v<T>;


	inline constexpr auto _memory_relocate_ = functor_cascade_t
	{
		[]<trivially_copyable T>(auto&&, auto&&, T* px, T* py, size_t count)
		{
			::memmove(px, py, sizeof(T) * count);
		},

		[]<move_constructible T>(auto&& dest_allocator, auto&& src_allocator, T* px, T* py, size_t count)
		{
			using dest_allocator_traits = allocator_traits_of_t<decltype(dest_allocator)>;
			using src_allocator_traits = allocator_traits_of_t<decltype(src_allocator)>;

			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != count; ++i, ++px, ++py)
				{
					dest_allocator_traits::construct(dest_allocator, px, std::move(*py));
					src_allocator_traits::destroy(src_allocator, py);
				}
			}
			// src range earlier, move backwards
			else
			{
				auto pxe = px + count;
				auto pye = py + count;
				for (size_t i = 0; i != count; ++i)
				{
					--pxe, --pye;

					dest_allocator_traits::construct(dest_allocator, pxe, std::move(*pye));
					src_allocator_traits::destroy(src_allocator, pye);
				}
			}
		},

		[]<copy_constructible T>(auto&& dest_allocator, auto&& src_allocator, T* px, T* py, size_t count)
		{
			using dest_allocator_traits = allocator_traits_of_t<decltype(dest_allocator)>;
			using src_allocator_traits = allocator_traits_of_t<decltype(src_allocator)>;

			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != count; ++i, ++px, ++py)
				{
					dest_allocator_traits::construct(dest_allocator, px, *py);
					src_allocator_traits::destroy(src_allocator, py);
				}
			}
			// src range earlier, move backwards
			else
			{
				auto pxe = px + count;
				auto pye = py + count;
				for (size_t i = 0; i != count; ++i)
				{
					--pxe, --pye;

					dest_allocator_traits::construct(dest_allocator, pxe, *pye);
					src_allocator_traits::destroy(src_allocator, pye);
				}
			}
		},

#if 1
		[]<typename T>(auto&& dest_allocator, auto&& src_allocator, T* px, T* py, size_t count)
		requires std::is_default_constructible_v<T> && std::is_move_assignable_v<T>
		{
			using dest_allocator_traits = allocator_traits_of_t<decltype(dest_allocator)>;
			using src_allocator_traits = allocator_traits_of_t<decltype(src_allocator)>;

			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != count; ++i, ++px, ++py)
				{
					dest_allocator_traits::construct(dest_allocator, px);
					*px = static_cast<T&&>(*py);
					src_allocator_traits::destroy(src_allocator, py);
				}
			}
			// src range earlier, move backwards
			else if (py < px)
			{
				auto pxe = px + count;
				auto pye = py + count;

				while (pxe != px)
				{
					--pxe, --pye;

					dest_allocator_traits::construct(dest_allocator, pxe);
					*pxe = std::move(*pye);
					src_allocator_traits::destroy(src_allocator, pye);
				}
			}
		},
#endif

#if 0
		[]<template T>(auto&& dest_allocator, auto&& src_allocator, T* px, T* py, size_t size)
		requires std::is_default_constructible_v<T> && std::is_copy_assignable_v<T>
		{
			using dest_allocator_traits = allocator_traits_of_t<decltype(dest_allocator)>;
			using src_allocator_traits = allocator_traits_of_t<decltype(src_allocator)>;

			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != size; ++i, ++px, ++py)
				{
					dest_allocator_traits::construct(
						dest_allocator,
						px, std::move(*py));

					src_allocator_traits::destroy(
						src_allocator,
						py);
				}
			}
			// src range earlier, move backwards
			else
			{
				auto pxe = px + size;
				auto pye = py + size;
				for (size_t i = 0; i != size; ++i)
				{
					--pxe, --pye;

					dest_allocator_traits::construct(
						dest_allocator,
						pxe, std::move(*pye));

					src_allocator_traits::destroy(
						src_allocator,
						pye);
				}
			}
		},
#endif

		[](auto&& dest_allocator, auto* px, auto begin, auto end)
		requires std::contiguous_iterator<decltype(begin)>
		{
			for (; begin != end; ++px, ++begin)
			{
				construct_with_allocator_traits_(dest_allocator, px, std::move(*begin));
			}
		}
	};
}

export namespace atma
{
	inline constexpr auto memory_copy = detail::_memory_range_delegate_<decltype(detail::_memory_copy_)>;
	inline constexpr auto memory_move = detail::_memory_range_delegate_<decltype(detail::_memory_move_)>;
	inline constexpr auto memory_relocate = detail::_memory_range_delegate_<decltype(detail::_memory_relocate_)>;

	inline constexpr auto memory_copy_construct = detail::_memory_range_delegate_<decltype(detail::_memory_copy_construct_)>;
	inline constexpr auto memory_move_construct = detail::_memory_range_delegate_<decltype(detail::_memory_move_construct_)>;
}

export namespace atma
{
	inline auto memory_compare = functor_cascade_t
	{
		[](memory_concept auto&& lhs, memory_concept auto&& rhs, size_t sz)
		{
			constexpr bool lhs_is_bounded = bounded_memory_concept<decltype(lhs)>;
			constexpr bool rhs_is_bounded = bounded_memory_concept<decltype(rhs)>;

			ATMA_ASSERT(sz != unbounded_memory_size);

			if constexpr (lhs_is_bounded)
				ATMA_ASSERT(sz <= std::size(lhs));
			if constexpr (rhs_is_bounded)
				ATMA_ASSERT(sz <= std::size(rhs));

			return ::memcmp(std::data(lhs), std::data(rhs), sz);
		},

		[](bounded_memory_concept auto&& lhs, bounded_memory_concept auto&& rhs)
		{
			auto const lhs_sz = std::size(lhs);
			auto const rhs_sz = std::size(rhs);

			if (lhs_sz < rhs_sz)
			{
				auto r = ::memcmp(std::data(lhs), std::data(rhs), lhs_sz);
				return r ? r : -(int)lhs_sz;
			}
			else if (rhs_sz < lhs_sz)
			{
				auto r = ::memcmp(std::data(lhs), std::data(rhs), rhs_sz);
				return r ? r : (int)rhs_sz;
			}
			else
			{
				return ::memcmp(std::data(lhs), std::data(rhs), lhs_sz);
			}
		}
	};
}
