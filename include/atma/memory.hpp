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

namespace atma
{
	constexpr struct memory_allocate_copy_tag {} memory_allocate_copy;
	constexpr struct memory_take_ownership_tag {} memory_take_ownership;

	struct dest_memory_tag_t;
	struct src_memory_tag_t;
	constexpr size_t unbounded_memory_size = ~size_t();

	template <typename Tag> struct unbounded_size_t;

	template <typename Tag, typename T, typename A = std::allocator<T>>
	struct memxfer_range_t;

	//
	// convertible if:
	//  - same tag, obviously
	//  - To is the unbounded version of From
	//    (bounded -> unbounded is fine, not the other way around)
	//  - dest-range is being used as a source-range
	//  - a bounded dest-range is being used as an unbounded source-range
	//
	template <typename To, typename From>
	constexpr bool is_convertible_xfer_v
		=  std::is_same_v<To, From>
		|| std::is_same_v<To, unbounded_size_t<From>>
		|| std::is_same_v<src_memory_tag_t, To> && std::is_same_v<dest_memory_tag_t, From>
		|| std::is_same_v<unbounded_size_t<src_memory_tag_t>, To> && std::is_same_v<dest_memory_tag_t, From>
		;
}

#define ATMA_ASSERT_MEMORY_RANGES_DISJOINT(lhs, rhs) \
	ATMA_ASSERT(std::addressof(*std::end(lhs)) <= std::addressof(*std::begin(rhs)) || \
		std::addressof(*std::end(rhs)) <= std::addressof(*std::begin(lhs)))

#define ATMA_ASSERT_MEMORY_DISJOINT(lhs, rhs, sz) \
	ATMA_ASSERT((lhs).data() + sz <= (rhs).data() || (rhs).data() + sz <= (lhs).data())

#define ATMA_ASSERT_MEMORY_PTR_DISJOINT(lhs, rhs, sz) \
	ATMA_ASSERT((lhs) + sz <= (rhs) || (rhs) + sz <= (lhs))

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

		base_memory_tx()
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			: allocator_(rhs.allocator())
		{}

		base_memory_tx(Allocator const& allocator)
			: allocator_(allocator)
		{}

		auto allocator() const -> allocator_type& { return allocator_; }

	private:
		Allocator allocator_;
	};


	template <typename Allocator>
	struct base_memory_tx<Allocator, true>
		: protected Allocator
	{
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		base_memory_tx()
		{}

		template <typename U>
		base_memory_tx(base_memory_tx<U> const& rhs)
			: Allocator(rhs.allocator())
		{}

		base_memory_tx(Allocator const& allocator)
			: Allocator(allocator)
		{}

		auto allocator() const -> allocator_type& { return const_cast<allocator_type&>(static_cast<allocator_type const&>(*this)); }
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

		basic_memory_t() = default;
		explicit basic_memory_t(allocator_type const&);
		explicit basic_memory_t(pointer, allocator_type const& = allocator_type());
		basic_memory_t(memory_allocate_copy_tag, const_pointer, size_t size, allocator_type const& = allocator_type());
		explicit basic_memory_t(size_t capacity, allocator_type const& = allocator_type());
		template <typename B> basic_memory_t(basic_memory_t<T, B> const&);

		auto operator = (basic_memory_t const&) -> basic_memory_t& = default;
		auto operator = (pointer) -> basic_memory_t&;
		template <typename B> auto operator = (basic_memory_t<T, B> const&) -> basic_memory_t&;

		operator bool() const { return ptr_ != nullptr; }
		operator pointer() const { return ptr_; }
		operator const_pointer() const { return ptr_; }

		auto operator *  () const -> reference;
		auto operator [] (intptr) const -> reference;
		auto operator -> () const -> pointer;

		auto data() const -> pointer { return ptr_; }

		// allocator interface
		auto allocate(size_t) -> bool;
		auto deallocate(size_t = 0) -> void;

	protected:
		value_type* ptr_ = nullptr;
	};


	// deduction guides
	template <typename T>
	basic_memory_t(T*) -> basic_memory_t<T>;

	template <typename T, typename A>
	basic_memory_t(T*, A const&) -> basic_memory_t<T, A>;


	// addition
	template <typename T, typename A, typename D, CONCEPT_MODELS_(integral_concept, D)>
	inline auto operator + (basic_memory_t<T, A> lhs, D d)
	{
		return basic_memory_t<T, A>(lhs.data() + d, lhs.allocator());
	}
}


namespace atma
{
	using memory_t = basic_memory_t<byte>;
}




namespace atma
{
	template <typename T, typename A>
	inline basic_memory_t<T, A>::basic_memory_t(allocator_type const& allocator)
		: base_type(allocator)
	{}

	template <typename T, typename A>
	inline basic_memory_t<T, A>::basic_memory_t(pointer data, allocator_type const& alloc)
		: base_type(alloc)
		, ptr_(data)
	{}

	template <typename T, typename A>
	template <typename B>
	inline basic_memory_t<T, A>::basic_memory_t(basic_memory_t<T, B> const& rhs)
		: detail::base_memory_t<T, A>(rhs.allocator())
		, ptr_(rhs.ptr_)
	{}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::operator = (pointer rhs) -> basic_memory_t &
	{
		ptr_ = rhs;
		return *this;
	}

	template <typename T, typename A>
	template <typename B>
	inline auto basic_memory_t<T, A>::operator = (basic_memory_t<T, B> const& rhs) -> basic_memory_t &
	{
		ptr_ = rhs.ptr_;
		this->allocator() = rhs.allocator();
		return *this;
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::operator *  () const -> reference
	{
		return *ptr_;
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::operator [] (intptr idx) const -> reference
	{
		return ptr_[idx];
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::operator -> () const -> pointer
	{
		return ptr_;
	}
}


namespace atma
{
	template <typename T, typename A>
	inline basic_memory_t<T, A>::basic_memory_t(memory_allocate_copy_tag, const_pointer data, size_t size, allocator_type const& alloc)
		: base_type(alloc)
	{
		allocate(size);
		std::memcpy(this->ptr_, data, size * sizeof(value_type));
	}

	template <typename T, typename A>
	inline basic_memory_t<T, A>::basic_memory_t(size_t capacity, allocator_type const& alloc)
		: base_type(alloc)
	{
		allocate(capacity);
	}

#if 0
	template <typename T, typename A>
	inline auto allocatable_memory_t<T, A>::operator = (pointer rhs) -> allocatable_memory_t&
	{
		this->ptr_ = rhs;
		return *this;
	}

	template <typename T, typename A>
	template <typename B>
	inline auto allocatable_memory_t<T, A>::operator = (allocatable_memory_t<T, B> const& rhs) -> allocatable_memory_t&
	{
		this->ptr_ = rhs.ptr_;
		this->allocator() = rhs.allocator();
		return *this;
	}
#endif

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::allocate(size_t size) -> bool
	{
		this->ptr_= allocator_traits::allocate(this->allocator(), size);
		return this->ptr_ != nullptr;
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::deallocate(size_t size) -> void
	{
		allocator_traits::deallocate(this->allocator(), this->ptr_, size);
	}

}



// concept: memory-range
namespace atma
{
	template <typename R, typename = void_t<>>
	struct has_allocator_retrieval : std::false_type
	{};

	template <typename R>
	struct has_allocator_retrieval<R, void_t<decltype(std::declval<R>().allocator())>>
		: std::true_type
	{
		static decltype(auto) retrieve_allocator(R&& r) { return r.allocator(); }
	};

	template <typename R>
	struct has_allocator_retrieval<R, void_t<decltype(std::declval<R>().get_allocator())>>
		: std::true_type
	{
		static decltype(auto) retrieve_allocator(R&& r) { return r.get_allocator(); }
	};

	template <typename R>
	constexpr bool has_allocator_retrieval_v = has_allocator_retrieval<R>::value;

	template <typename R>
	inline decltype(auto) retrieve_allocator(R&& r)
	{
		return has_allocator_retrieval<R>::retrieve_allocator(std::forward<R>(r));
	}

	struct memory_concept
	{
		template <typename Memory>
		auto contract(Memory memory) -> concepts::specifies
		<
			// range has allocator/data
			SPECIFIES_TYPE(typename Memory::value_type),
			SPECIFIES_TYPE(typename Memory::allocator_type),
			SPECIFIES_EXPR(std::data(memory)),
			has_allocator_retrieval_v<Memory>
		>;
	};

	struct bounded_memory_concept
		: concepts::refines<memory_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies
		<
			// range can provide size
			SPECIFIES_EXPR(std::size(std::declval<Memory>()))
		>;
	};

	struct dest_memory_concept
		: concepts::refines<memory_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies<
			std::is_same_v<dest_memory_tag_t, typename Memory::tag_type>
		>;
	};

	struct src_memory_concept
		: concepts::refines<memory_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies<
			std::is_same_v<src_memory_tag_t, typename Memory::tag_type>
		>;
	};

	struct dest_bounded_memory_concept
		: concepts::refines<bounded_memory_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies<
			std::is_same_v<dest_memory_tag_t, typename Memory::tag_type>
		>;
	};

	struct src_bounded_memory_concept
		: concepts::refines<bounded_memory_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies<
			std::is_same_v<src_memory_tag_t, typename Memory::tag_type>
		>;
	};

	struct sized_range_concept
		: concepts::refines<range_concept>
	{
		template <typename Range>
		auto contract(Range&& range) -> concepts::specifies
		<
			// range can provide size
			SPECIFIES_EXPR(std::size(range))
		>;
	};

	
	struct value_type_copy_constructible_concept
	{
		template <typename Memory>
		auto contract() -> concepts::specifies
		<
			// we can assign/copy-construct to elements in this range
			//concepts::is_true<concepts::models<assignable_concept, typename std::remove_reference_t<Memory>::value_type>>,
			concepts::models_v<copy_constructible_concept, typename Memory::value_type>
		>;
	};
}


//
//
// memxfer_range_t
// -----------------
//   a type used for transferring memory around.
//
//   this structure contains a pointer to some memory and possibly a
//   stateful allocator. stateless allocators add no additional size to
//   this type, so it's usually just the size of a pointer. stateful
//   allocators will make the size of this type grow.
//
//
namespace atma
{
	template <typename Tag, typename T, typename A>
	struct memxfer_range_t
	{
		using value_type = T;
		using allocator_type = A;
		using tag_type = Tag;

		// default-constructor only allowed if allocator doesn't hold state
		CONCEPT_REQUIRES(std::is_empty_v<allocator_type>)
		constexpr memxfer_range_t()
			: alloc_and_ptr_(allocator_type(), nullptr)
		{}

		constexpr memxfer_range_t(allocator_type a, T* ptr)
			: alloc_and_ptr_(a, ptr)
		{}

		CONCEPT_REQUIRES(std::is_empty_v<allocator_type>)
		constexpr memxfer_range_t(T* ptr)
			: alloc_and_ptr_(allocator_type(), ptr)
		{}

		~memxfer_range_t() = default;

		allocator_type& allocator() const { return const_cast<memxfer_range_t*>(this)->alloc_and_ptr_.first(); }

		auto data() -> value_type* { return alloc_and_ptr_.second(); }
		auto data() const -> value_type const* { return alloc_and_ptr_.second(); }

	protected:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T* const, first_as_reference_transformer_t>;
		alloc_and_ptr_type alloc_and_ptr_;
	};
}


//
//
// bounded_memxfer_range_t
// -----------------
//   a type used for transferring memory around.
//
//   this structure is just a memxfer_range_t but with a size. this allows 
//   it to have empty(), size(), begin(), and end() methods. this allows
//   us options for optimization in some algorithms
//
//
namespace atma
{
	template <typename Tag, typename T, typename A>
	struct bounded_memxfer_range_t : memxfer_range_t<Tag, T, A>
	{
		using base_type = memxfer_range_t<Tag, T, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;
		using tag_type = Tag;

		// inherit constructors
		using base_type::base_type;

		// default-constructor only allowed if allocator doesn't hold state
		CONCEPT_REQUIRES(std::is_empty_v<allocator_type>)
		constexpr bounded_memxfer_range_t()
		{}

		constexpr bounded_memxfer_range_t(allocator_type allocator, T* ptr, size_t size)
			: base_type(allocator, ptr)
			, size_(size)
		{}

		CONCEPT_REQUIRES(std::is_empty_v<allocator_type>)
		constexpr bounded_memxfer_range_t(T* ptr, size_t size)
			: base_type(allocator_type(), ptr)
			, size_(size)
		{}

		auto begin()       -> value_type* { return this->alloc_and_ptr_.second(); }
		auto end()         -> value_type* { return this->alloc_and_ptr_.second() + size_; }
		auto begin() const -> value_type const* { return this->alloc_and_ptr_.second(); }
		auto end() const   -> value_type const* { return this->alloc_and_ptr_.second() + size_; }

		auto empty() const -> bool { return size_ == 0; }
		auto size() const -> size_t { return size_; }
		auto byte_size() const -> size_t { return size_ * sizeof value_type; }

	private:
		size_t const size_ = unbounded_memory_size;
	};
}


// dest/src versions
namespace atma
{
	// dest_memxfer_range_t
	template <typename T, typename A>
	using dest_memxfer_range_t = memxfer_range_t<dest_memory_tag_t, T, A>;

	template <typename T, typename A>
	using dest_bounded_memxfer_range_t = bounded_memxfer_range_t<dest_memory_tag_t, T, A>;

	// src_memxfer_range_t
	template <typename T, typename A>
	using src_memxfer_range_t = memxfer_range_t<src_memory_tag_t, T, A>;

	template <typename T, typename A>
	using src_bounded_memxfer_range_t = bounded_memxfer_range_t<src_memory_tag_t, T, A>;

	// memxfer_range_of
	template <typename Tag, typename Range>
	using memxfer_range_of_t = memxfer_range_t<Tag, value_type_of_t<Range>, allocator_type_of_t<Range>>;

	template <typename Tag, typename Range>
	using bounded_memxfer_range_of_t = bounded_memxfer_range_t<Tag, value_type_of_t<Range>, allocator_type_of_t<Range>>;
}


// implementations of how to make dest/src ranges from various arguments
namespace atma::detail
{
	template <typename tag_type>
	struct xfer_make_from_ptr_
	{
		template <typename T>
		auto operator()(T* data) const
		{
			return memxfer_range_t<tag_type, T, std::allocator<T>>(data);
		}

		template <typename T>
		auto operator()(T* data, size_t sz) const
		{
			return bounded_memxfer_range_t<tag_type, T, std::allocator<T>>(data, sz);
		}
	};

	template <typename tag_type>
	struct xfer_make_from_sized_contiguous_range_
	{
		template <typename R,
			CONCEPT_MODELS_(contiguous_range_concept, rmref_t<R>),
			CONCEPT_MODELS_(sized_range_concept, rmref_t<R>)>
		auto operator ()(R&& range) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			if constexpr (has_allocator_retrieval_v<R>)
				return {retrieve_allocator(range), std::addressof(*std::begin(range)), std::size(range)};
			else
				return {std::addressof(*std::begin(range)), std::size(range)};
		};
	};

	template <typename tag_type>
	struct xfer_make_from_contiguous_range_
	{
		template <typename R,
			CONCEPT_MODELS_(contiguous_range_concept, rmref_t<R>)>
		auto operator ()(R&& range) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			auto const sz = std::distance(std::begin(range), std::end(range));

			if constexpr (has_allocator_retrieval_v<decltype(range)>)
				return {retrieve_allocator(range), std::addressof(*std::begin(range)), sz};
			else
				return {std::addressof(*std::begin(range)), sz};
		}

		template <typename R,
			CONCEPT_MODELS_(contiguous_range_concept, rmref_t<R>)>
		auto operator ()(R&& range, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			if constexpr (has_allocator_retrieval_v<decltype(range)>)
				return {retrieve_allocator(range), std::addressof(*std::begin(range)), size};
			else
				return {std::addressof(*std::begin(range)), size};
		}

		template <typename R,
			CONCEPT_MODELS_(contiguous_range_concept, rmref_t<R>)>
		auto operator ()(R&& range, size_t offset, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<R>>
		{
			if constexpr (has_allocator_retrieval_v<decltype(range)>)
				return {retrieve_allocator(range), std::addressof(*std::begin(range)) + offset, size};
			else
				return {std::addressof(*std::begin(range)) + offset, size};
		}
	};

	template <typename tag_type>
	struct xfer_make_from_memory_
	{
		template <typename M,
			CONCEPT_MODELS_(memory_concept, rmref_t<M>)>
		auto operator ()(M&& memory) -> memxfer_range_of_t<tag_type, rmref_t<M>>
		{
			return {retrieve_allocator(memory), std::data(memory)};
		}

		template <typename M,
			CONCEPT_MODELS_(memory_concept, rmref_t<M>)>
		auto operator ()(M&& memory, size_t size) -> bounded_memxfer_range_of_t<tag_type, rmref_t<M>>
		{
			return {retrieve_allocator(memory), std::data(memory), size};
		}
	};

	template <typename type_tag>
	struct xfer_make_from_iterator_pair_
	{
		template <typename It,
			CONCEPT_MODELS_(contiguous_iterator_concept, It)>
		auto operator ()(It begin, It end)
			-> bounded_memxfer_range_t
			< type_tag
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
	using xfer_range_maker_ = multi_functor_t
	<
		functor_call_no_fwds_t,

		// first match against pointer
		xfer_make_from_ptr_<tag_type>,

		// then match against that satisfies the contiguous-range concept (std::begin,
		// std::end), AND satisfies the sized-range concept (std::size)
		xfer_make_from_sized_contiguous_range_<tag_type>,

		// then try _only_ the contiguous-range concept (use std::distance instead of std::size)
		xfer_make_from_contiguous_range_<tag_type>,

		// then try anything that satisfies the memory concept (retrieve_allocator & std::data)
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
	using allocator_traits_of_range_t = std::allocator_traits<std::allocator<rm_cvref_t<decltype(*std::begin(std::declval<R>()))>>>;

	template <typename R>
	constexpr auto allocator_type_of_range_(R&& a) -> std::allocator<rmref_t<decltype(*std::begin(a))>>;

	template <typename A>
	constexpr auto allocator_traits_of_allocator_(A&& a) -> std::allocator_traits<rmref_t<A>>;
}

// memory_construct_at
namespace atma
{
	constexpr auto memory_construct_at = [](auto&& dest, auto&&... args)
		-> RETURN_TYPE_IF(void, MODELS_ARGS(memory_concept, dest))
	{
		decltype(auto) allocator = retrieve_allocator(dest);
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
	constexpr auto _memory_range_delegate = multi_functor_t
	{
		functor_call_fwds_t<F>{},

		[](auto& f, auto&& dest, size_t sz)
		LAMBDA_REQUIRES(MODELS_ARGS(dest_memory_concept, dest))
			{ f(retrieve_allocator(dest), std::data(dest), sz); },

		[](auto& f, auto&& dest)
		LAMBDA_REQUIRES(MODELS_ARGS(dest_bounded_memory_concept, dest))
			{ f(retrieve_allocator(dest), std::data(dest), std::size(dest)); }
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
		using dest_allocator_traits = std::allocator_traits<rmref_t<decltype(allocator)>>;

		for (size_t i = 0; i != sz; ++i)
			dest_allocator_traits::construct(allocator, px++, args...);
	};
}

namespace atma
{
	constexpr auto memory_construct = multi_functor_t
	{
		[] (auto&& dest, auto&&... args)
			-> RETURN_TYPE_IF(void,
				MODELS_ARGS(dest_memory_concept, dest))
		{
			detail::_memory_range_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::size(dest),
				std::forward<decltype(args)>(args)...);
		},

		[](auto&& dest, auto&&... args)
			-> RETURN_TYPE_IF(void,
				MODELS_ARGS(sized_range_concept, dest))
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
	constexpr auto _memory_copy_construct = [](auto&& allocator, auto* px, auto* py, size_t sz)
	{
		if (sz == 0)
			return;

		ATMA_ASSERT_MEMORY_PTR_DISJOINT(px, py, sz);

		using dest_allocator_traits = std::allocator_traits<rmref_t<decltype(allocator)>>;

		for (size_t i = 0; i != sz; ++i)
			dest_allocator_traits::construct(allocator, px++, *py++);
	};
}

namespace atma::detail
{
}

namespace atma
{
	constexpr auto memory_copy_construct = multi_functor_t
	{
		[](auto&& dest, auto&& src, size_t sz)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(src_memory_concept, src))
		{
			constexpr bool dest_is_bounded = concepts::models_ref_v<bounded_memory_concept, decltype(dest)>;
			constexpr bool src_is_bounded = concepts::models_ref_v<bounded_memory_concept, decltype(src)>;

			ATMA_ASSERT(sz != unbounded_memory_size);
			
			if constexpr (dest_is_bounded && src_is_bounded)
				ATMA_ASSERT(dest.size() == src.size());

			if constexpr (dest_is_bounded)
				ATMA_ASSERT(dest.size() == sz);

			if constexpr (src_is_bounded)
				ATMA_ASSERT(src.size() == sz);
				
			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_bounded_memory_concept, dest),
			MODELS_ARGS(src_bounded_memory_concept, src))
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_bounded_memory_concept, dest),
			MODELS_ARGS(src_memory_concept, src))
		{
			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(src_bounded_memory_concept, src))
		{
			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(src));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(contiguous_range_concept, src))
		{
			auto const sz = std::distance(std::begin(src), std::end(src));

			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::addressof(*std::begin(src)),
				sz);
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(sized_range_concept, dest),
			MODELS_ARGS(sized_range_concept, src))
		{
			using allocator_type = decltype(detail::allocator_type_of_range_(dest));

			detail::_memory_copy_construct(
				allocator_type(),
				std::data(dest),
				std::data(src),
				std::size(src));
		},

		[](auto&& dest, auto begin, auto end)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(iterator_concept, begin),
			MODELS_ARGS(iterator_concept, end))
		{
			using dest_allocator_traits = std::allocator_traits<allocator_type_of_t<rmref_t<decltype(dest)>>>;

			auto px = std::data(dest);
			while (begin != end)
				dest_allocator_traits::construct(retrieve_allocator(dest), px++, *begin++);
		}

	};
}

// memory_move_construct
namespace atma::detail
{
	constexpr auto _memory_move_construct = [](auto&& allocator, auto* px, auto* py, size_t sz)
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
	};
}

namespace atma
{
	constexpr auto memory_move_construct = multi_functor_t
	{
		[](auto&& dest, auto&& src, size_t sz)
			-> RETURN_TYPE_IF(void,
				MODELS_ARGS(dest_memory_concept, dest),
				MODELS_ARGS(src_memory_concept, src))
		{
			if (sz == 0)
				return;

			using dest_type = rmref_t<decltype(dest)>;
			constexpr bool dest_is_bounded = concepts::models_v<bounded_memory_concept, dest_type>;

			using src_type = rmref_t<decltype(src)>;
			constexpr bool src_is_bounded = concepts::models_v<bounded_memory_concept, src_type>;

			ATMA_ASSERT(sz != unbounded_memory_size);

			if constexpr (dest_is_bounded && src_is_bounded)
			{
				ATMA_ASSERT(dest.size() == src.size());
				ATMA_ASSERT_MEMORY_RANGES_DISJOINT(dest, src);
			}

			if constexpr (dest_is_bounded)
				ATMA_ASSERT(dest.size() == sz);

			if constexpr (src_is_bounded)
				ATMA_ASSERT(src.size() == sz);

			using dest_allocator_traits = std::allocator_traits<allocator_type_of_t<decltype(dest)>>;

			detail::_memory_move_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto&& dest, auto&& src) -> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(src_memory_concept, src),
			MODELS_ARGS(bounded_memory_concept, dest))
		{
			detail::_memory_move_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src) -> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(src_memory_concept, src),
			MODELS_ARGS(bounded_memory_concept, src))
		{
			detail::_memory_move_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::data(src),
				std::size(src));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(contiguous_range_concept, src))
		{
			auto const sz = std::distance(std::begin(src), std::end(src));

			detail::_memory_copy_construct(
				retrieve_allocator(dest),
				std::data(dest),
				std::addressof(*std::begin(src)),
				sz);
		},
	};
}

// destruct
namespace atma
{
	template <typename DT, typename DA>
	inline auto memory_destruct(dest_bounded_memxfer_range_t<DT, DA> dest_range) -> void
	{
		for (auto& x : dest_range)
			std::allocator_traits<DA>::destroy(dest_range.allocator(), &x);
	}
}

// memcpy / memmove
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range, size_t byte_size) -> void
	{
		static_assert(sizeof DT == sizeof ST);

		ATMA_ASSERT(byte_size != unbounded_memory_size);
		ATMA_ASSERT(byte_size % sizeof DT == 0);
		ATMA_ASSERT(byte_size % sizeof ST == 0);

		std::memcpy(dest_range.data(), src_range.data(), byte_size);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.byte_size() == src_range.byte_size());
		auto sz = dest_range.byte_size();
		memcpy(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = dest_range.byte_size();
		memcpy(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = src_range.byte_size();
		memcpy(dest_range, src_range, sz);
	}

#if 1
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range, size_t byte_size) -> void
	{
		static_assert(sizeof DT == sizeof ST);

		ATMA_ASSERT(byte_size != unbounded_memory_size);
		ATMA_ASSERT(byte_size % sizeof DT == 0);
		ATMA_ASSERT(byte_size % sizeof ST == 0);

		std::memmove(dest_range.data(), src_range.data(), byte_size);
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
	inline auto memmove(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.byte_size() == src_range.byte_size());
		auto sz = dest_range.byte_size();
		memmove(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = dest_range.byte_size();
		memmove(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = src_range.byte_size();
		memmove(dest_range, src_range, sz);
	}

}

// memfill
namespace atma::memory
{
	template <typename DT, typename DA, typename V>
	inline auto memfill(dest_bounded_memxfer_range_t<DT, DA> dest_range, V v) -> void
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
	constexpr auto memory_relocate_range = multi_functor_t
	{
		[] (auto&& dest, auto&& src, size_t sz)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_memory_concept, dest),
			MODELS_ARGS(src_memory_concept, src))
		{
			constexpr bool dest_is_bounded = concepts::models_ref_v<bounded_memory_concept, decltype(dest)>;
			constexpr bool src_is_bounded = concepts::models_ref_v<bounded_memory_concept, decltype(src)>;

			ATMA_ASSERT(sz != unbounded_memory_size);

			if constexpr (dest_is_bounded && src_is_bounded)
				ATMA_ASSERT(dest.size() == src.size());

			if constexpr (dest_is_bounded)
				ATMA_ASSERT(dest.size() == sz);

			if constexpr (src_is_bounded)
				ATMA_ASSERT(src.size() == sz);

			detail::_memory_relocate_range(
				retrieve_allocator(dest),
				retrieve_allocator(src),
				std::data(dest),
				std::data(src),
				sz);
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(dest_bounded_memory_concept, dest),
			MODELS_ARGS(src_bounded_memory_concept, src))
		{
			ATMA_ASSERT(std::size(dest) == std::size(src));

			detail::_memory_relocate_range(
				retrieve_allocator(dest),
				retrieve_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(bounded_memory_concept, dest),
			MODELS_ARGS(memory_concept, src))
		{
			detail::_memory_relocate_range(
				retrieve_allocator(dest),
				retrieve_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(dest));
		},

		[](auto&& dest, auto&& src)
		-> RETURN_TYPE_IF(void,
			MODELS_ARGS(memory_concept, dest),
			MODELS_ARGS(bounded_memory_concept, src))
		{
			detail::_memory_relocate_range(
				retrieve_allocator(dest),
				retrieve_allocator(src),
				std::data(dest),
				std::data(src),
				std::size(src));
		},
	};
	}
