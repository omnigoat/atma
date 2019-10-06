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

	struct memory_dest_tag_t;
	struct memory_src_tag_t;
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
		|| std::is_same_v<memory_src_tag_t, To> && std::is_same_v<memory_dest_tag_t, From>
		|| std::is_same_v<unbounded_size_t<memory_src_tag_t>, To> && std::is_same_v<memory_dest_tag_t, From>
		;
}

#define ATMA_ASSERT_MEMORY_RANGES_DISJOINT(lhs, rhs) \
	ATMA_ASSERT(std::addressof(*std::end(lhs)) <= std::addressof(*std::begin(rhs)) || \
		std::addressof(*std::end(rhs)) <= std::addressof(*std::begin(lhs)))


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
	inline auto basic_memory_t<T, A>::allocate(size_t count) -> bool
	{
		this->ptr_= allocator_traits::allocate(this->allocator(), count);
		return this->ptr_ != nullptr;
	}

	template <typename T, typename A>
	inline auto basic_memory_t<T, A>::deallocate(size_t count) -> void
	{
		allocator_traits::deallocate(this->allocator(), this->ptr_, count);
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

	struct sized_range_concept
	{
		template <typename Memory>
		auto contract() -> concepts::specifies
		<
			// range can provide size
			SPECIFIES_EXPR(std::size(std::declval<Memory>()))
		>;
	};

	struct memory_concept
	{
		template <typename Memory>
		auto contract(Memory memory) -> concepts::specifies
		<
			// range has allocator/data
			SPECIFIES_TYPE(typename Memory::value_type),
			SPECIFIES_TYPE(typename Memory::allocator_type),
			SPECIFIES_EXPR(memory.data()),
			has_allocator_retrieval_v<Memory>
		>;
	};

	struct sized_memory_concept
		: concepts::refines<memory_concept, sized_range_concept>
	{};

	struct sized_memory_range_concept
		: concepts::refines<memory_concept, range_concept, sized_range_concept>
	{};

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



// memxfer_range_t
namespace atma
{
	template <typename Tag, typename T, typename A>
	struct memxfer_range_t
	{
		using value_type = T;
		using allocator_type = A;

		// converting construct allows bounded ranges to be used as unbounded,
		// because that is a less restrictive
		template <typename RTag, typename RA>
		memxfer_range_t(memxfer_range_t<RTag, T, RA> rhs)
			: alloc_and_ptr_(rhs.allocator(), rhs.data())
		{}

		// default-constructor only allowed if allocator doesn't hold state
		CONCEPT_REQUIRES(std::is_empty_v<A>)
		memxfer_range_t()
			: alloc_and_ptr_(allocator_type(), nullptr)
		{}

		// Pointer
		memxfer_range_t(allocator_type a, T* ptr)
			: alloc_and_ptr_(a, ptr)
		{}

		memxfer_range_t(T* ptr)
			: alloc_and_ptr_(allocator_type(), ptr)
		{}

		// Memory (Range + .allocator())
		template <typename M, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
		memxfer_range_t(M memory)
			: alloc_and_ptr_(memory.allocator(), memory.data())
		{}

		~memxfer_range_t() = default;

		allocator_type& allocator() const { return const_cast<memxfer_range_t*>(this)->alloc_and_ptr_.first(); }

		T* data() { return alloc_and_ptr_.second(); }
		T const* data() const { return alloc_and_ptr_.second(); }

	protected:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T* const, first_as_reference_transformer_t>;
		alloc_and_ptr_type alloc_and_ptr_;
	};

	template <typename Tag, typename T, typename A>
	memxfer_range_t(memxfer_range_t<Tag, T, A>) -> memxfer_range_t<unbounded_size_t<Tag>, T, A>;

}

namespace atma
{
	template <typename Tag, typename T, typename A>
	struct bounded_memxfer_range_t : memxfer_range_t<Tag, T, A>
	{
		using base_type = memxfer_range_t<Tag, T, A>;
		using value_type = typename base_type::value_type;
		using allocator_type = typename base_type::allocator_type;

		// inherit constructors
		using base_type::base_type;

		//
		// Pointer
		//
		bounded_memxfer_range_t(T* ptr, size_t size)
			: base_type(allocator_type(), ptr)
			, size_(size)
		{}


		//
		// Generic Range (vector, array, etc)
		//
		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		bounded_memxfer_range_t(Range&& range)
			: base_type(allocator_type(), &*std::begin(std::forward<Range>(range)))
			, size_(std::distance(std::begin(std::forward<Range>(range)), std::end(std::forward<Range>(range))))
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		bounded_memxfer_range_t(Range&& range, size_t size)
			: base_type(allocator_type(), &*std::begin(range))
			, size_(size)
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		bounded_memxfer_range_t(Range&& range, size_t offset, size_t size)
			: base_type(allocator_type(), &*std::begin(range) + offset)
			, size_(size)
		{}

		//
		// Memory (Range + .allocator())
		//
		template <typename M, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
		bounded_memxfer_range_t(M memory, size_t size)
			: base_type(memory.allocator(), memory.data())
			, size_(size)
		{}

		//
		// Iterator Pair
		//
		template <typename It, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, It)>
		bounded_memxfer_range_t(It begin, It end)
			: base_type(allocator_type(), &*begin)
			, size_(std::distance(begin, end))
		{}



		value_type* begin() { return this->alloc_and_ptr_.second(); }
		value_type* end() { return this->alloc_and_ptr_.second() + size_; }

		value_type const* begin() const { return this->alloc_and_ptr_.second(); }
		value_type const* end() const { return this->alloc_and_ptr_.second() + size_; }

		bool empty() const { return size_ == 0; }
		size_t size() const { return size_; }
		size_t byte_size() const { return size_ * sizeof value_type; }

	private:
		size_t const size_ = unbounded_memory_size;
	};
}


// dest_range
namespace atma
{
	template <typename T, typename A>
	using dest_memxfer_range_t = memxfer_range_t<memory_dest_tag_t, T, A>;

	template <typename T, typename A>
	using dest_bounded_memxfer_range_t = bounded_memxfer_range_t<memory_dest_tag_t, T, A>;

	

	// pointer
	template <typename T, typename... Args>
	inline auto dest_range(T* data)
		-> dest_memxfer_range_t<T, std::allocator<T>>
	{
		static_assert(!std::is_const_v<T>);
		return dest_memxfer_range_t{data};
	}

	template <typename T, typename... Args>
	inline auto dest_range(T* data, size_t size)
		-> dest_bounded_memxfer_range_t<T, std::allocator<T>>
	{
		static_assert(!std::is_const_v<T>);
		return {data, size};
	}

	// random-access-range
	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	inline auto dest_range(Range&& range)
		-> dest_bounded_memxfer_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>
	{
		return {std::forward<Range>(range), std::size(std::forward<Range>(range))};
	}

	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	inline auto dest_range(Range&& range, size_t size)
		-> dest_bounded_memxfer_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>
	{
		return {std::forward<Range>(range), size};
	}

	// "memory"
	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, rmref_t<M>), CONCEPT_NOT_MODELS_(sized_range_concept, rmref_t<M>)>
	inline auto dest_range(M&& memory)
		-> dest_memxfer_range_t<value_type_of_t<M>, allocator_type_of_t<M>>
	{
		return {std::forward<M>(memory)};
	}

	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, rmref_t<M>), CONCEPT_NOT_MODELS_(sized_range_concept, rmref_t<M>)>
	inline auto dest_range(M&& memory, size_t size)
		-> dest_bounded_memxfer_range_t<value_type_of_t<M>, allocator_type_of_t<M>>
	{
		return {std::forward<M>(memory), size};
	}

	// iterator-pair
	template <typename It, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, It)>
	inline auto dest_range(It begin, It end)
		-> dest_bounded_memxfer_range_t
		< std::remove_reference_t<decltype(*std::declval<It>())>
		, std::allocator<std::remove_reference_t<decltype(*std::declval<It>())>>>
	{
		return {begin, end};
	}
}

// src_range
namespace atma
{
	template <typename T, typename A = std::allocator<T>>
	using src_memxfer_range_t = memxfer_range_t<memory_src_tag_t, T, A>;

	template <typename T, typename A = std::allocator<T>>
	using src_bounded_memxfer_range_t = bounded_memxfer_range_t<memory_src_tag_t, T, A>;



	// pointer
	template <typename T, typename... Args>
	inline auto src_range(T* data)
		-> src_memxfer_range_t<T, std::allocator<T>>
	{
		static_assert(!std::is_const_v<T>);
		return {data};
	}

	template <typename T, typename... Args>
	inline auto src_range(T* data, size_t size)
		-> src_bounded_memxfer_range_t<T, std::allocator<T>>
	{
		static_assert(!std::is_const_v<T>);
		return {data, size};
	}

	// random-access-range
	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	inline auto src_range(Range&& range)
		-> src_bounded_memxfer_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>
	{
		return {std::forward<Range>(range), std::size(std::forward<Range>(range))};
	}

	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	inline auto src_range(Range&& range, size_t size)
		-> src_bounded_memxfer_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>
	{
		return {std::forward<Range>(range), size};
	}

	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	inline auto src_range(Range&& range, size_t offset, size_t size)
		-> src_bounded_memxfer_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>
	{
		return {std::forward<Range>(range), offset, size};
	}

	// "memory"
	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, rmref_t<M>), CONCEPT_NOT_MODELS_(sized_range_concept, rmref_t<M>)>
	inline auto src_range(M&& memory)
		-> src_memxfer_range_t<value_type_of_t<M>, allocator_type_of_t<M>>
	{
		return {std::forward<M>(memory)};
	}

	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, rmref_t<M>)>
	inline auto src_range(M&& memory, size_t size)
		-> src_bounded_memxfer_range_t<value_type_of_t<M>, allocator_type_of_t<M>>
	{
		return {std::forward<M>(memory), size};
	}

	// iterator-pair
	template <typename It, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, It)>
	inline auto src_range(It begin, It end)
		-> src_bounded_memxfer_range_t
		< std::remove_reference_t<decltype(*std::declval<It>())>
		, std::allocator<std::remove_const_t<std::remove_reference_t<decltype(*std::declval<It>())>>>>
	{
		return {begin, end};
	}





// construct
namespace atma::memory
{
	template <typename T, typename A, typename... Args>
	inline auto construct(basic_memory_t<T, A> const& ptr, Args&&... args) -> void
	{
		basic_memory_t<T, A>::allocator_traits::construct(ptr.allocator(), (T*)ptr, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	inline auto construct(T* ptr, Args&&... args) -> void
	{
		new (ptr) T(std::forward<Args>(args)...);
	}
}

// construct_range
namespace atma::memory
{
	template <typename T, typename A, typename... Args>
	inline auto range_construct(dest_bounded_memxfer_range_t<T, A> range, Args&&... args) -> void
	{
		for (auto& x : range)
			std::allocator_traits<A>::construct(range.allocator(), &x, std::forward<Args>(args)...);
	}
}

// copy_construct_range
namespace atma::memory2
{
#if 0
	ATMA_ASSERT_MEMORY_RANGES_DISJOINT(
		(dest_memxfer_range_t<DT, DA>(dest.begin(), dest.begin() + sz)),
		(src_range_t<ST, SA>(src.begin(), src.begin() + sz)));
#endif
	constexpr auto _range_copy_construct = [](auto&& dest, auto&& src, size_t sz)
	{
		using allocator_type = allocator_type_of_t<rmref_t<decltype(dest)>>;

		auto px = std::begin(dest);
		auto py = std::begin(src);
		for (size_t i = 0; i != sz; ++i)
			std::allocator_traits<allocator_type>::construct(retrieve_allocator(dest), px++, *py++);
	};

	constexpr auto range_copy_construct = make_functor(
		[](auto&& dest, auto&& src, size_t sz)
			-> RETURN_TYPE_IF(void,
				MODELS_ARGS(memory_concept, dest),
				MODELS_ARGS(memory_concept, src))
		{
			if (sz == 0)
				return;

			using dest_type = rmref_t<decltype(dest)>;
			constexpr bool is_dest_sized = concepts::models_v<sized_memory_concept, dest_type>;

			using src_type = rmref_t<decltype(src)>;
			constexpr bool is_src_sized = concepts::models_v<sized_memory_concept, src_type>;

			ATMA_ASSERT(sz != unbounded_memory_size);

			if constexpr (is_dest_sized && is_src_sized)
			{
				ATMA_ASSERT(dest.size() == src.size());

				ATMA_ASSERT_MEMORY_RANGES_DISJOINT(
					dest_range(dest.begin(), dest.end()),
					src_range(src.begin(), src.end()));
			}
			
			if constexpr (is_dest_sized)
			{
				ATMA_ASSERT(dest.size() == sz);
			}
			
			if constexpr (is_src_sized)
			{
				ATMA_ASSERT(src.size() == sz);
			}

			using allocator_type = allocator_type_of_t<decltype(dest)>;

			auto px = std::begin(dest);
			auto py = std::begin(src);
			for (size_t i = 0; i != sz; ++i)
				std::allocator_traits<allocator_type>::construct(retrieve_allocator(dest), px++, *py++);
		},
		[](auto&& dest, auto&& src)
			-> RETURN_TYPE_IF(void,
				concepts::models_v<sized_memory_concept, rmref_t<decltype(dest)>>,
				concepts::models_v<sized_memory_concept, rmref_t<decltype(src)>>)
		{
			ATMA_ASSERT(dest.size() == src.size());

			range_copy_construct(
				std::forward<decltype(dest)>(dest),
				std::forward<decltype(src)>(src),
				dest.size());
		});
}

namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_copy_construct(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);
		//ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		//ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		//ATMA_ASSERT_MEMORY_RANGES_DISJOINT(
		//	(dest_memxfer_range_t<DT, DA>(dest_range.begin(), dest_range.begin() + sz)),
		//	(src_memxfer_range_t<ST, SA>(src_range.begin(), src_range.begin() + sz)));

		auto px = dest_range.data();
		auto py = src_range.data();
		for (size_t i = 0; i != sz; ++i)
			std::allocator_traits<DA>::construct(dest_range.allocator(), px++, *py++);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_copy_construct(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		range_copy_construct<DT, DA, ST, SA>(dest_range, src_range, sz);
	}

	template <typename DR, typename SR,
		CONCEPT_MODELS_(sized_memory_range_concept, rmref_t<DR>),
		CONCEPT_MODELS_(sized_memory_range_concept, rmref_t<SR>),
		CONCEPT_MODELS_(copy_constructible_concept, typename rmref_t<DR>::value_type)>
	inline auto range_copy_construct(DR&& dest_range, SR&& src_range) -> void
	{
		ATMA_ASSERT(std::size(dest_range) == std::size(src_range));

		using dest_alloc_traits = std::allocator_traits<allocator_type_of_t<DR>>;

		decltype(auto) alloc = retrieve_allocator(dest_range);
		auto py = std::begin(src_range);
		for (auto& x : dest_range)
			dest_alloc_traits::construct(alloc, &x, *py++);
	}

	template <typename DR, typename It,
		//CONCEPT_MODELS_(memory_concept, std::remove_reference_t<DR>),
		//CONCEPT_MODELS_(value_type_copy_constructible_concept, DR),
		CONCEPT_MODELS_(concepts::forward_iterator_concept, It)>
		inline auto range_copy_construct(DR&& dest_range, It begin) -> void
	{
		using dest_alloc_traits = std::allocator_traits<allocator_type_of_t<DR>>;

		decltype(auto) alloc = retrieve_allocator(dest_range);
		for (auto& x : dest_range)
			dest_alloc_traits::construct(alloc, &x, *begin++);
	}
}

// relocate_range
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto relocate_range(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);

		auto px = dest_range.data();
		auto py = src_range.data();

		// destination range is earlier, move forwards
		if (px < py)
		{
			for (size_t i = 0; i != sz; ++i, ++px, ++py)
			{
				std::allocator_traits<DA>::construct(dest_range.allocator(), px, std::move(*py));
				std::allocator_traits<SA>::destroy(src_range.allocator(), py);
			}
		}
		// src range earlier, move backwards
		else
		{
			auto pxe = px + sz;
			auto pye = py + sz;
			for (size_t i = 0; i != sz; ++i)
			{
				--pxe, --pye;
				std::allocator_traits<DA>::construct(dest_range.allocator(), pxe, std::move(*pye));
				std::allocator_traits<SA>::destroy(src_range.allocator(), pye);
			}
		}
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto relocate_range(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		relocate_range(dest_range, src_range, sz);
	}
}

// range_move_construct
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_move_construct(dest_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);

		auto px = dest_range.data();
		auto py = src_range.data();

		// destination range is earlier, move forwards
		if (px < py)
		{
			for (size_t i = 0; i != sz; ++i, ++px, ++py)
			{
				std::allocator_traits<DA>::construct(dest_range.allocator(), px, std::move(*py));
			}
		}
		// src range earlier, move backwards
		else
		{
			auto pxe = px + sz;
			auto pye = py + sz;
			for (size_t i = 0; i != sz; ++i)
			{
				std::allocator_traits<DA>::construct(dest_range.allocator(), --pxe, std::move(*--pye));
			}
		}
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_move_construct(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.size() == src_range.size());

		auto sz = dest_range.size();
		range_move_construct(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_move_construct(dest_bounded_memxfer_range_t<DT, DA> dest_range, src_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = dest_range.size();
		range_move_construct(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_move_construct(dest_memxfer_range_t<DT, DA> dest_range, src_bounded_memxfer_range_t<ST, SA> src_range) -> void
	{
		auto sz = src_range.size();
		range_move_construct(dest_range, src_range, sz);
	}

	template <typename T, typename A, typename SrcIt>
	inline auto range_move_construct(dest_memxfer_range_t<T, A> dest_range, SrcIt src_begin, SrcIt src_end) -> void
	{
		auto px = (T*)dest_range.ptr + dest_range.idx;

		using dest_alloc_traits = std::allocator_traits<A>;
		while (dest_range.size --> 0 && src_begin != src_end)
		{
			dest_alloc_traits::construct(dest_range.allocator, px++, std::move(*src_begin));
			++src_begin;
		}
	}
}

// destruct
namespace atma::memory
{
	template <typename DT, typename DA>
	inline auto range_destruct(dest_bounded_memxfer_range_t<DT, DA> dest_range) -> void
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
