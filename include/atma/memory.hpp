#pragma once

#include <atma/aligned_allocator.hpp>
#include <atma/concepts.hpp>
#include <atma/assert.hpp>
#include <atma/types.hpp>
#include <atma/ebo_pair.hpp>
#include <atma/functor.hpp>

#include <atma/ranges/core.hpp>

#include <type_traits>
#include <vector>
#include <memory>




//
// thoughts on memory things
//
//  a structure that combines an allocator & a pointer
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
namespace atma
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
namespace atma::detail
{
	template <typename Allocator, bool = std::is_empty_v<Allocator>>
	struct base_memory_tx;

	template <typename Allocator>
	struct base_memory_tx<Allocator, false>
	{
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		base_memory_tx() = default;
		base_memory_tx(base_memory_tx const&) = default;

		base_memory_tx(Allocator const& allocator)
			noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
			: allocator_(allocator)
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			noexcept(noexcept(Allocator(rhs.get_allocator())))
			: allocator_(rhs.get_allocator())
		{}

		auto get_allocator() const -> allocator_type { return allocator_; }

	private:
		Allocator allocator_;
	};

	template <typename Allocator>
	struct base_memory_tx<Allocator, true>
		: protected Allocator
	{
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		base_memory_tx() = default;
		base_memory_tx(base_memory_tx const&) = default;

		base_memory_tx(Allocator const& allocator)
			: Allocator(allocator)
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			noexcept(noexcept(Allocator(rhs.get_allocator())))
			: Allocator(rhs.get_allocator())
		{}

		auto get_allocator() const -> allocator_type { return static_cast<allocator_type const&>(*this); }
	};

	template <typename T, typename A>
	using base_memory_t = base_memory_tx<typename std::allocator_traits<A>::template rebind_alloc<T>>;
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
		return stream << "memory{0x" << x.ptr << "}";
	}
}

namespace atma
{
	using memory_t = basic_memory_t<byte>;
}



//
//  unique_memory_t
//  -----------------
//
//
//
namespace atma
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

namespace atma
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
namespace atma
{
	constexpr auto get_allocator = functor_list_t
	{
		[](auto&& r) -> decltype(r.get_allocator()) { return r.get_allocator(); },
		[](auto&& r) { return std::allocator<rm_cvref_t<value_type_of_t<decltype(r)>>>(); }
	};
}

// concept: memory-range
namespace atma
{
	template <typename Memory>
	concept memory_concept = requires(Memory memory)
	{
		typename rmref_t<Memory>::value_type;
		typename rmref_t<Memory>::allocator_type;
		{ std::data(memory) };
		{ atma::get_allocator(memory) };
	};

	template <typename Memory>
	concept bounded_memory_concept = memory_concept<Memory> &&
	requires(Memory memory)
	{
		{ std::size(memory) };
	};

	template <typename Memory>
	concept dest_memory_concept = memory_concept<Memory> &&
		std::is_same_v<dest_memory_tag_t, typename rmref_t<Memory>::tag_type>;

	template <typename Memory>
	concept src_memory_concept = memory_concept<Memory> &&
		std::is_same_v<src_memory_tag_t, typename rmref_t<Memory>::tag_type>;

	template <typename Memory>
	concept dest_bounded_memory_concept = bounded_memory_concept<Memory> &&
		std::is_same_v<dest_memory_tag_t, typename rmref_t<Memory>::tag_type>;

	template <typename Memory>
	concept src_bounded_memory_concept = bounded_memory_concept<Memory> &&
		std::is_same_v<src_memory_tag_t, typename rmref_t<Memory>::tag_type>;
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

		// default-constructor only allowed if allocator doesn't hold state
		constexpr memxfer_t(T* ptr = nullptr)
		requires std::is_empty_v<allocator_type>
			: alloc_and_ptr_(allocator_type(), ptr)
		{}

		constexpr memxfer_t(allocator_type a, T* ptr)
			: alloc_and_ptr_(a, ptr)
		{}

		~memxfer_t() = default;

		allocator_type& get_allocator() const { return const_cast<memxfer_t*>(this)->alloc_and_ptr_.first(); }

		auto data() -> value_type* { return alloc_and_ptr_.second(); }
		auto data() const -> value_type const* { return alloc_and_ptr_.second(); }

		auto operator [](size_t idx) const -> value_type&
		{
			return data()[idx];
		}

	protected:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T* const>;
		alloc_and_ptr_type alloc_and_ptr_;
	};
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
namespace atma
{
	template <typename Tag, typename T, typename A>
	struct bounded_memxfer_t : memxfer_t<Tag, T, A>
	{
		using base_type = memxfer_t<Tag, T, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;
		using tag_type = Tag;

		constexpr static size_t dynamic_extent = ~size_t();

		bounded_memxfer_t(bounded_memxfer_t const& rhs)
			: base_type(rhs.get_allocator(), rhs.data())
			, size_(rhs.size())
		{}

		// we, a range of const-elements, can always adapt a range of non-const elements
		bounded_memxfer_t(bounded_memxfer_t<Tag, std::remove_const_t<T>, A> const& rhs)
		requires std::is_const_v<T>
			: base_type(rhs.get_allocator(), (T const*)rhs.data())
			, size_(rhs.size())
		{}

		constexpr bounded_memxfer_t(allocator_type allocator, T* ptr, size_t size)
			: base_type(allocator, ptr)
			, size_(size)
		{}

		constexpr bounded_memxfer_t(T* ptr, size_t size)
		requires std::is_empty_v<allocator_type>
			: base_type(allocator_type(), ptr)
			, size_(size)
		{}

		auto begin()       -> value_type* { return this->alloc_and_ptr_.second(); }
		auto end()         -> value_type* { return this->alloc_and_ptr_.second() + size_; }
		auto begin() const -> value_type const* { return this->alloc_and_ptr_.second(); }
		auto end() const   -> value_type const* { return this->alloc_and_ptr_.second() + size_; }

		auto empty() const -> bool { return size_ == 0; }
		auto size() const -> size_t { return size_; }
		auto size_bytes() const -> size_t { return size_ * sizeof value_type; }

		constexpr auto subspan(size_t offset, size_t count = dynamic_extent) const { 
			return bounded_memxfer_t(this->get_allocator(), this->data() + offset, (count == dynamic_extent) ? size() - offset : count); }

		// immutable transforms
		auto skip(size_t n) const -> bounded_memxfer_t<Tag, T, A>
		{
			ATMA_ASSERT(n < size_);
			return bounded_memxfer_t<Tag, T, A>(this->get_allocator(), this->data() + n, this->size_ - n);
		}

	private:
		size_t const size_ = unbounded_memory_size;
	};
}


//
// dest_/src_ versions of memory-ranges
// --------------------------
//
//   SERIOUSLY write some docos
//
namespace atma
{
	// dest_memxfer_t
	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using dest_memxfer_t = memxfer_t<dest_memory_tag_t, T, A>;

	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using dest_bounded_memxfer_t = bounded_memxfer_t<dest_memory_tag_t, T, A>;

	// src_memxfer_t
	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using src_memxfer_t = memxfer_t<src_memory_tag_t, T, A>;

	template <typename T, typename A = std::allocator<std::remove_const_t<T>>>
	using src_bounded_memxfer_t = bounded_memxfer_t<src_memory_tag_t, T, A>;
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
	using bounded_memxfer_range_of_t = bounded_memxfer_t<Tag, value_type_of_t<Range>, allocator_type_of_t<Range>>;


	template <typename tag_type>
	struct xfer_make_from_ptr_
	{
		template <typename T>
		auto operator()(T* data) const
		{
			return memxfer_t<tag_type, T, std::allocator<std::remove_const_t<T>>>(data);
		}

		template <typename T, typename A>
		auto operator()(A&& allocator, T* data) const
		{
			return memxfer_t<tag_type, T, rmref_t<A>>(std::forward<A>(allocator), data);
		}

		template <typename T>
		auto operator()(T* data, size_t sz) const
		{
			return bounded_memxfer_t<tag_type, T, std::allocator<std::remove_const_t<T>>>(data, sz);
		}

		template <typename T, typename A>
		auto operator()(A&& allocator, T* data, size_t sz) const
		{
			return bounded_memxfer_t<tag_type, T, rmref_t<A>>(std::forward<A>(allocator), data, sz);
		}
	};

	template <typename tag_type>
	struct xfer_make_from_sized_contiguous_range_
	{
		template <typename R>
		requires std::ranges::sized_range<R> &&
		         std::ranges::contiguous_range<R>
		auto operator ()(R&& range) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			return {get_allocator(range), std::addressof(*std::begin(range)), std::size(range)};
		};
	};

	template <typename tag_type>
	struct xfer_make_from_contiguous_range_
	{
		template <typename R>
		requires std::ranges::contiguous_range<R>
		auto operator ()(R&& range) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			auto const sz = std::distance(std::begin(range), std::end(range));

			return {get_allocator(range), std::addressof(*std::begin(range)), sz};
		}

		template <typename R>
		requires std::ranges::contiguous_range<R>
		auto operator ()(R&& range, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			return {get_allocator(range), std::addressof(*std::begin(range)), size};
		}

		template <typename R>
		requires std::ranges::contiguous_range<R>
		auto operator ()(R&& range, size_t offset, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			return {get_allocator(range), std::addressof(*std::begin(range)) + offset, size};
		}
	};

	template <typename tag_type>
	struct xfer_make_from_memory_
	{
		template <typename M>
		requires memory_concept<M>
		auto operator ()(M&& memory) -> memxfer_range_of_t<tag_type, rmref_t<M>>
		{
			return {get_allocator(memory), std::data(memory)};
		}

		template <typename M>
		requires memory_concept<M>
		auto operator ()(M&& memory, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<M>>
		{
			return {get_allocator(memory), std::data(memory), size};
		}
	};

	template <typename Tag>
	struct xfer_make_from_iterator_pair_
	{
		template <typename It>
		requires std::contiguous_iterator<It>
		auto operator ()(It begin, It end)
			-> bounded_memxfer_t
			< Tag
			, std::remove_reference_t<decltype(*std::declval<It>())>
			, std::allocator<std::remove_const_t<std::remove_reference_t<decltype(*std::declval<It>())>>>>
		{
			size_t const sz = std::distance(begin, end);

			return {std::addressof(*begin), sz};
		}
	};
}

namespace atma::detail
{
	template <typename tag_type>
	using xfer_range_maker_ = functor_list_t
	<
		functor_call_no_fwds_t,

		// first match against pointer
		xfer_make_from_ptr_<tag_type>,

		// then match anything that satisfies the contiguous-range concept (std::begin,
		// std::end), AND satisfies the sized-range concept (std::size)
		xfer_make_from_sized_contiguous_range_<tag_type>,

		// then try _only_ the contiguous-range concept (use std::distance instead of std::size)
		xfer_make_from_contiguous_range_<tag_type>,

		// then try anything that satisfies the memory concept (get_allocator & std::data)
		xfer_make_from_memory_<tag_type>,

		// iterator-pair version
		xfer_make_from_iterator_pair_<tag_type>
	>;
}

namespace atma
{
	constexpr auto xfer_dest = detail::xfer_range_maker_<dest_memory_tag_t>();
	constexpr auto xfer_src  = detail::xfer_range_maker_<src_memory_tag_t>();
}





namespace atma::detail
{
	template <typename R>
	constexpr auto allocator_type_of_range_(R&& a) -> std::allocator<rm_cvref_t<decltype(*std::begin(a))>>;

	template <typename A>
	constexpr auto allocator_traits_of_allocator_(A&& a) -> std::allocator_traits<rmref_t<A>>;
}






//##################################
//
//  begin algorithms here
//
//##################################






// memory_construct_at
namespace atma
{
	constexpr auto memory_construct_at = [](auto&& dest, auto&&... args)
	requires memory_concept<decltype(dest)>
	{
		decltype(auto) allocator = get_allocator(dest);
		using allocator_traits = std::allocator_traits<rmref_t<decltype(allocator)>>;

		allocator_traits::construct(allocator, std::data(dest), std::forward<decltype(args)>(args)...);
	};
}

// memory_default_construct / memory_value_construct
namespace atma::detail
{
	constexpr auto _memory_default_construct = [](auto&&, auto* px, size_t sz)
	{
		using value_type = rmref_t<decltype(*px)>;

		// allocator is not capable of default-constructing
		for (size_t i = 0; i != sz; ++i)
			::new (px++) value_type;
	};

	constexpr auto _memory_value_construct = [](auto&& allocator, auto* px, size_t sz)
	{
		using allocator_traits = decltype(allocator_traits_of_allocator_(allocator));
		for (size_t i = 0; i != sz; ++i)
			allocator_traits::construct(allocator, px++);
	};

	template <typename F>
	constexpr auto _memory_range_delegate = functor_list_t
	{
		functor_call_fwds_t<F>{},

		[](auto& f, auto&& dest)
		requires dest_bounded_memory_concept<decltype(dest)>
		{
			f(get_allocator(dest), std::data(dest), std::size(dest));
		},

		[](auto& f, auto&& dest, size_t sz)
		requires dest_memory_concept<decltype(dest)>
		{
			f(get_allocator(dest), std::data(dest), sz);
		}
	};
}

namespace atma
{
	constexpr auto memory_default_construct = detail::_memory_range_delegate<decltype(detail::_memory_default_construct)>;
	constexpr auto memory_value_construct = detail::_memory_range_delegate<decltype(detail::_memory_value_construct)>;
}


// memory_construct
namespace atma::detail
{
	constexpr auto _memory_range_construct = [](auto&& allocator, auto* px, size_t sz, auto&&... args)
	{
		using dest_allocator_traits = decltype(allocator_traits_of_allocator_(allocator));
		
		for (size_t i = 0; i != sz; ++i)
			dest_allocator_traits::construct(allocator, px++, args...);
	};
}

namespace atma
{
	constexpr auto memory_construct = functor_list_t
	{
		[](auto&& dest, auto&&... args)
		requires dest_memory_concept<decltype(dest)>
		{
			detail::_memory_range_construct(
				get_allocator(dest),
				std::data(dest),
				std::size(dest),
				std::forward<decltype(args)>(args)...);
		},

		[](auto&& dest, auto&&... args)
		requires std::ranges::sized_range<decltype(dest)>
		{
			using allocator_type = decltype(detail::allocator_type_of_range_(dest));

			detail::_memory_range_construct(
				allocator_type(),
				std::addressof(*std::begin(dest)),
				std::size(dest),
				std::forward<decltype(args)>(args)...);
		}
	};
}



// memory_copy_construct / memory_move_construct
namespace atma::detail
{
	constexpr auto _memory_copy_construct = functor_list_t
	{
		[](auto&& allocator, auto* px, auto* py, size_t sz)
		{
			ATMA_ASSERT(sz == 0 || (px + sz <= py) || (py + sz <= px),
				"memory ranges must be disjoint");

			using dest_allocator_traits = decltype(allocator_traits_of_allocator_(allocator));

			for (size_t i = 0; i != sz; ++i)
				dest_allocator_traits::construct(allocator, px++, *py++);
		},

		[](auto&& allocator, auto* px, auto begin, auto end)
		{
			using dest_allocator_traits = std::allocator_traits<rmref_t<decltype(allocator)>>;

			while (begin != end)
				dest_allocator_traits::construct(allocator, px++, *begin++);
		}
	};

	constexpr auto _memory_move_construct = functor_list_t
	{
		[](auto&& allocator, auto* px, auto* py, size_t sz)
		{
			if (sz == 0)
				return;

			using dest_allocator_traits = std::allocator_traits<rmref_t<decltype(allocator)>>;

			// destination range is earlier, move forwards
			if (px < py)
			{
				for (size_t i = 0; i != sz; ++i, ++px, ++py)
				{
					dest_allocator_traits::construct(allocator, px, std::move(*py));
				}
			}
			// src range earlier, move backwards
			else
			{
				auto pxe = px + sz;
				auto pye = py + sz;
				for (size_t i = 0; i != sz; ++i)
				{
					dest_allocator_traits::construct(allocator, --pxe, std::move(*--pye));
				}
			}
		},

		[](auto&& allocator, auto* px, auto begin, auto end)
		{
			using dest_allocator_traits = decltype(allocator_traits_of_allocator_(allocator));

			while (begin != end)
				dest_allocator_traits::construct(allocator, px, std::move(*begin++));
		}
	};
}

namespace atma::detail
{
#define MEMORY_TYPES(dest_concept, src_concept) \
	dest_concept<decltype(dest)> && src_concept<decltype(src)>

	template <typename F>
	constexpr auto _memory_copymove_ = functor_list_t
	{
		functor_call_fwds_t<F>(),

		[](auto& f, auto&& dest, auto&& src, size_t sz)
		requires MEMORY_TYPES(dest_memory_concept, src_memory_concept)
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

			f(get_allocator(dest),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto& f, auto&& dest, auto&& src)
		requires MEMORY_TYPES(dest_bounded_memory_concept, src_bounded_memory_concept)
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			f(get_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto& f, auto&& dest, auto&& src)
		requires MEMORY_TYPES(dest_bounded_memory_concept, src_memory_concept)
		{
			f(get_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto& f, auto&& dest, auto&& src)
		requires MEMORY_TYPES(dest_memory_concept, src_bounded_memory_concept)
		{
			f(get_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(src));
		},

		[](auto&& dest, auto&& src)
		requires MEMORY_TYPES(dest_memory_concept, std::ranges::contiguous_range)
		{
			auto const sz = std::distance(std::begin(src), std::end(src));

			f(get_allocator(dest),
				std::data(dest),
				std::addressof(*std::begin(src)),
				sz);
		},

		[](auto& f, auto&& dest, auto&& src)
		requires MEMORY_TYPES(std::ranges::sized_range, std::ranges::sized_range)
		{
			using allocator_type = decltype(detail::allocator_type_of_range_(dest));

			f(allocator_type(),
				std::data(dest),
				std::data(src),
				std::size(src));
		},
		
		[](auto& f, auto&& dest, auto begin, auto end)
		requires dest_memory_concept<decltype(dest)> &&
			std::input_iterator<decltype(begin)> &&
			std::equality_comparable_with<decltype(begin), decltype(end)>
		{
			f(get_allocator(dest),
				std::data(dest),
				begin, end);
		}
	};

#undef MEMORY_TYPES
}

namespace atma
{
	constexpr auto memory_copy_construct = detail::_memory_copymove_<decltype(detail::_memory_copy_construct)>;
	constexpr auto memory_move_construct = detail::_memory_copymove_<decltype(detail::_memory_move_construct)>;
}



// destruct
namespace atma
{
	template <typename DT, typename DA>
	inline auto memory_destruct(dest_bounded_memxfer_t<DT, DA> dest_range) -> void
	{
		for (auto& x : dest_range)
			std::allocator_traits<DA>::destroy(dest_range.get_allocator(), &x);
	}
}

// memcpy / memmove
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_memxfer_t<DT, DA> dest_range, src_memxfer_t<ST, SA> src_range, size_t size_bytes) -> void
	{
		static_assert(sizeof DT == sizeof ST);

		ATMA_ASSERT(size_bytes != unbounded_memory_size);
		ATMA_ASSERT(size_bytes % sizeof DT == 0);
		ATMA_ASSERT(size_bytes % sizeof ST == 0);

		std::memcpy(dest_range.data(), src_range.data(), size_bytes);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_bounded_memxfer_t<DT, DA> dest_range, src_bounded_memxfer_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.size_bytes() == src_range.size_bytes());
		auto sz = dest_range.size_bytes();
		memcpy(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_bounded_memxfer_t<DT, DA> dest_range, src_memxfer_t<ST, SA> src_range) -> void
	{
		auto sz = dest_range.size_bytes();
		memcpy(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_memxfer_t<DT, DA> dest_range, src_bounded_memxfer_t<ST, SA> src_range) -> void
	{
		auto sz = src_range.size_bytes();
		memcpy(dest_range, src_range, sz);
	}

#if 1
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_memxfer_t<DT, DA> dest_range, src_memxfer_t<ST, SA> src_range, size_t size_bytes) -> void
	{
		static_assert(sizeof DT == sizeof ST);

		ATMA_ASSERT(size_bytes != unbounded_memory_size);
		ATMA_ASSERT(size_bytes % sizeof DT == 0);
		ATMA_ASSERT(size_bytes % sizeof ST == 0);

		std::memmove(dest_range.data(), src_range.data(), size_bytes);
	}

#else
	template <typename D, typename S>
	inline auto memmove(D&& dest, S&& src, size_t sz) -> void
	{
		using DR = std::remove_reference_t<D>;
		using SR = std::remove_reference_t<S>;

		//if constexpr (concepts::models_v<)
		//std::is_invocable_v<

		ATMA_ASSERT(sz != unbounded_memory_size);
		//ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		//ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		std::memmove(dest_range.begin(), src_range.begin(), sz * sizeof(DT));
	}
#endif

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_bounded_memxfer_t<DT, DA> dest_range, src_bounded_memxfer_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.size_bytes() == src_range.size_bytes());
		auto sz = dest_range.size_bytes();
		memmove(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_bounded_memxfer_t<DT, DA> dest_range, src_memxfer_t<ST, SA> src_range) -> void
	{
		auto sz = dest_range.size_bytes();
		memmove(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_memxfer_t<DT, DA> dest_range, src_bounded_memxfer_t<ST, SA> src_range) -> void
	{
		auto sz = src_range.size_bytes();
		memmove(dest_range, src_range, sz);
	}

}

// memfill
namespace atma::memory
{
	template <typename DT, typename DA, typename V>
	inline auto memfill(dest_bounded_memxfer_t<DT, DA> dest_range, V v) -> void
	{
		std::fill_n(dest_range.begin(), dest_range.size(), v);
	}

}




// relocate_range
namespace atma::detail
{
	constexpr auto _memory_relocate_range = [](auto&& dest_allocator, auto&& src_allocator, auto* px, auto* py, size_t size)
	{
		using dest_allocator_traits = decltype(allocator_traits_of_allocator_(dest_allocator));
		using src_allocator_traits = decltype(allocator_traits_of_allocator_(src_allocator));

		// destination range is earlier, move forwards
		if (px < py)
		{
			for (size_t i = 0; i != size; ++i, ++px, ++py)
			{
				dest_allocator_traits::construct(dest_allocator, px, std::move(*py));
				src_allocator_traits::destroy(src_allocator, py);
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
				dest_allocator_traits::construct(dest_allocator, pxe, std::move(*pye));
				src_allocator_traits::destroy(src_allocator, pye);
			}
		}
	};
}

namespace atma
{
	constexpr auto memory_relocate_range = functor_list_t
	{
		[] (auto&& dest, auto&& src, size_t sz)
		requires dest_memory_concept<decltype(dest)> && src_memory_concept<decltype(src)>
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

			detail::_memory_relocate_range(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto&& dest, auto&& src)
		requires dest_bounded_memory_concept<decltype(dest)>&& src_bounded_memory_concept<decltype(src)>
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			detail::_memory_relocate_range(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		requires dest_bounded_memory_concept<decltype(dest)>&& src_memory_concept<decltype(src)>
		{
			detail::_memory_relocate_range(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		requires dest_memory_concept<decltype(dest)>&& src_bounded_memory_concept<decltype(src)>
		{
			detail::_memory_relocate_range(
				get_allocator(dest),
				get_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(src));
		},
	};
}
