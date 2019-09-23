#pragma once

#include <atma/aligned_allocator.hpp>
#include <atma/concepts.hpp>
#include <atma/assert.hpp>
#include <atma/types.hpp>
#include <atma/ebo_pair.hpp>

#include <atma/ranges/core.hpp>

#include <type_traits>
#include <vector>
#include <memory>

namespace atma
{
	constexpr struct memory_allocate_copy_tag {} memory_allocate_copy;
	constexpr struct memory_take_ownership_tag {} memory_take_ownership;

	constexpr struct memory_dest_tag_t {} memory_dest_tag;
	constexpr struct memory_src_tag_t {} memory_src_tag;
	constexpr size_t unbounded_memory_size = ~size_t();

	template <typename T, typename A = std::allocator<T>> struct dest_range_t;
	template <typename T, typename A = std::allocator<T>> struct src_range_t;
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
	template <typename T, typename A, typename D, CONCEPT_MODELS_(atma::concepts::integral, D)>
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
	constexpr auto has_allocator_retrieval_v = has_allocator_retrieval<R>::value;

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
		auto contract() -> concepts::specifies
		<
			// range has allocator/data
			SPECIFIES_TYPE(typename std::remove_reference_t<Memory>::value_type),
			SPECIFIES_TYPE(typename std::remove_reference_t<Memory>::allocator_type),
			SPECIFIES_EXPR(std::declval<Memory&>().data()),
			has_allocator_retrieval<Memory>
		>;
	};

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
			concepts::is_true<concepts::models<copy_constructible_concept, typename std::remove_reference_t<Memory>::value_type>>
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

		//
		// default-constructor only allowed if allocator doesn't hold state
		//
		template <typename A2 = A, typename = std::enable_if_t<std::is_empty_v<A2>>>
		memxfer_range_t()
			: alloc_and_ptr_(allocator_type(), nullptr)
			, size_(unbounded_memory_size)
		{}

		//
		// Pointer
		//
		memxfer_range_t(T* ptr, size_t size)
			: alloc_and_ptr_(allocator_type(), ptr)
			, size_(size)
		{}

		memxfer_range_t(T* ptr, size_t idx, size_t size)
			: alloc_and_ptr_(allocator_type(), ptr + idx)
			, size_(size)
		{}

		//
		// Generic Range
		//
		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		memxfer_range_t(Range&& range)
			: alloc_and_ptr_(allocator_type(), &*std::begin(std::forward<Range>(range)))
			, size_(std::distance(std::begin(std::forward<Range>(range)), std::end(std::forward<Range>(range))))
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		memxfer_range_t(Range&& range, size_t size)
			: alloc_and_ptr_(allocator_type(), &*std::begin(range))
			, size_(size)
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		memxfer_range_t(Range&& range, size_t idx, size_t size)
			: alloc_and_ptr_(allocator_type(), &range[idx])
			, size_(size)
		{}

		//
		// Memory (Range + .allocator())
		//
		template <typename M, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
		memxfer_range_t(M memory)
			: alloc_and_ptr_(memory.allocator(), memory.data())
			, size_(unbounded_memory_size)
		{}

		template <typename M, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
		memxfer_range_t(M memory, size_t size)
			: alloc_and_ptr_(memory.allocator(), memory.data())
			, size_(size)
		{}

		template <typename M, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
		memxfer_range_t(M memory, size_t idx, size_t size)
			: alloc_and_ptr_(memory.allocator(), memory.data() + idx)
			, size_(size)
		{}

		//
		// Iterator Pair
		//
		template <typename It, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, It)>
		memxfer_range_t(It begin, It end)
			: alloc_and_ptr_(allocator_type(), &*begin)
			, size_(std::distance(begin, end))
		{}

		~memxfer_range_t() = default;

		allocator_type& allocator() const { return const_cast<memxfer_range_t*>(this)->alloc_and_ptr_.first(); }

		T* data() { return begin(); }
		T* begin() { return alloc_and_ptr_.second(); }
		T* end() { ATMA_ASSERT(!unbounded()); return alloc_and_ptr_.second() + size_; }
		
		T const* data() const { return begin(); }
		T const* begin() const { return alloc_and_ptr_.second(); }
		T const* end() const { ATMA_ASSERT(!unbounded()); return alloc_and_ptr_.second() + size_; }

		size_t size() const { return size_; }
		size_t bytesize() const { return size_ * sizeof T; }
		bool empty() const { return size_ == 0; }
		bool unbounded() const { return size_ == unbounded_memory_size; }

	private:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T* const, first_as_reference_transformer_t>;
		alloc_and_ptr_type alloc_and_ptr_;
		size_t const size_ = 0;
	};
}



// dest_range
namespace atma
{
	template <typename T, typename A>
	struct dest_range_t : memxfer_range_t<memory_dest_tag_t, T, A>
	{
		using memxfer_range_t<memory_dest_tag_t, T, A>::memxfer_range_t;
	};

	// deduction guides
	template <typename T, typename... Args>
	dest_range_t(T*, Args...) -> dest_range_t<T, std::allocator<T>>;

	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	dest_range_t(Range&&, Args...) -> dest_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>;

	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, M)>
	dest_range_t(M&&, Args...) -> dest_range_t<value_type_of_t<M>, allocator_type_of_t<M>>;

	template <typename I, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, I)>
	dest_range_t(I begin, I end) -> dest_range_t
		< std::remove_reference_t<decltype(*std::declval<I>())>
		, std::allocator<std::remove_reference_t<decltype(*std::declval<I>())>>
		>;
}

// src_range
namespace atma
{
	template <typename T, typename A>
	struct src_range_t : memxfer_range_t<memory_src_tag_t, T, A>
	{
		using memxfer_range_t<memory_src_tag_t, T, A>::memxfer_range_t;
	};

	// deduction guides
	template <typename T>
	src_range_t(T*) -> src_range_t<T, std::allocator<std::remove_const_t<T>>>;

	template <typename T>
	src_range_t(T*, size_t)->src_range_t<T, std::allocator<std::remove_const_t<T>>>;

	template <typename T>
	src_range_t(T*, size_t, size_t)->src_range_t<T, std::allocator<std::remove_const_t<T>>>;

	template <typename Range, typename... Args, CONCEPT_MODELS_(random_access_range_concept, Range)>
	src_range_t(Range&&, Args...) -> src_range_t<value_type_of_t<Range>, allocator_type_of_t<Range>>;

	template <typename M, typename... Args, CONCEPT_MODELS_(memory_concept, M), CONCEPT_NOT_MODELS_(sized_range_concept, M)>
	src_range_t(M&&, Args...) -> src_range_t<value_type_of_t<M>, allocator_type_of_t<M>>;

	template <typename I>
	src_range_t(I begin, I end) -> src_range_t
		< std::remove_reference_t<decltype(*std::declval<I>())>
		, std::allocator<std::remove_const_t<std::remove_reference_t<decltype(*std::declval<I>())>>>
		>;
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
	inline auto range_construct(dest_range_t<T, A> range, Args&&... args) -> void
	{
		for (auto& x : range)
			std::allocator_traits<A>::construct(range.allocator(), &x, std::forward<Args>(args)...);
	}
}

// copy_construct_range
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_copy_construct(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);
		ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		ATMA_ASSERT_MEMORY_RANGES_DISJOINT(
			(dest_range_t<DT, DA>(dest_range.begin(), dest_range.begin() + sz)),
			(src_range_t<ST, SA>(src_range.begin(), src_range.begin() + sz)));

		auto px = std::begin(dest_range);
		auto py = std::begin(src_range);
		for (size_t i = 0; i != sz; ++i)
			std::allocator_traits<DA>::construct(dest_range.allocator(), px++, *py++);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_copy_construct(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		range_copy_construct(dest_range, src_range, sz);
	}

	template <typename DR, typename SR,
		CONCEPT_MODELS_(sized_memory_range_concept, DR),
		CONCEPT_MODELS_(sized_memory_range_concept, SR),
		CONCEPT_MODELS_(value_type_copy_constructible_concept, DR)>
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
	inline auto relocate_range(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);
		ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		auto px = dest_range.begin();
		auto py = src_range.begin();

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
			auto pxe = dest_range.begin() + sz;
			auto pye = src_range.begin() + sz;
			for (size_t i = 0; i != sz; ++i)
			{
				--pxe, --pye;
				std::allocator_traits<DA>::construct(dest_range.allocator(), pxe, std::move(*pye));
				std::allocator_traits<SA>::destroy(src_range.allocator(), pye);
			}
		}
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto relocate_range(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
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
	inline auto range_move_construct(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range, size_t sz) -> void
	{
		if (sz == 0)
			return;

		ATMA_ASSERT(sz != unbounded_memory_size);
		ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		auto px = std::begin(dest_range);
		auto py = std::begin(src_range);

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
			auto pxe = dest_range.begin() + sz;
			auto pye = src_range.begin() + sz;
			for (size_t i = 0; i != sz; ++i)
			{
				std::allocator_traits<DA>::construct(dest_range.allocator(), --pxe, std::move(*--pye));
			}
		}
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto range_move_construct(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		range_move_construct(dest_range, src_range, sz);
	}

	template <typename T, typename A, typename SrcIt>
	inline auto range_move_construct(dest_range_t<T, A> dest_range, SrcIt src_begin, SrcIt src_end) -> void
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
	inline auto range_destruct(dest_range_t<DT, DA> dest_range) -> void
	{
		for (auto& x : dest_range)
			std::allocator_traits<DA>::destroy(dest_range.allocator(), &x);
	}
}

// memcpy / memmove
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range, size_t sz) -> void
	{
		static_assert(sizeof DT == sizeof ST);
		
		ATMA_ASSERT(sz != unbounded_memory_size);
		ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		std::memcpy(dest_range.begin(), src_range.begin(), sz * sizeof DT);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memcpy(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		memcpy(dest_range, src_range, sz);
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range, size_t sz) -> void
	{
		static_assert(sizeof DT == sizeof ST);

		ATMA_ASSERT(sz != unbounded_memory_size);
		ATMA_ASSERT(dest_range.unbounded() || dest_range.size() == sz);
		ATMA_ASSERT(src_range.unbounded() || src_range.size() == sz);

		std::memmove(dest_range.begin(), src_range.begin(), sz * sizeof(DT));
	}

	template <typename DT, typename DA, typename ST, typename SA>
	inline auto memmove(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(!dest_range.unbounded() || !src_range.unbounded());

		auto sz
			= !dest_range.unbounded() ? dest_range.size()
			: !src_range.unbounded() ? src_range.size()
			: unbounded_memory_size;

		memmove(dest_range, src_range, sz);
	}
}

// memfill
namespace atma::memory
{
	template <typename DT, typename DA, typename V>
	inline auto memcpy(dest_range_t<DT, DA> dest_range, V v) -> void
	{
		ATMA_ASSERT(dest_range.bytesize() % sizeof V == 0);

		std::fill_n(dest_range.begin(), dest_range.size(), v);
	}

}
