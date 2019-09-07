#pragma once

#include <atma/aligned_allocator.hpp>
#include <atma/concepts.hpp>
#include <atma/assert.hpp>
#include <atma/types.hpp>

#include <atma/ranges/core.hpp>

#include <type_traits>
#include <vector>
#include <memory>

namespace atma
{
	constexpr struct memory_allocate_copy_tag {} memory_allocate_copy;
	constexpr struct memory_take_ownership_tag {} memory_take_ownership;
}

// ebo-pair
namespace atma
{
	namespace detail
	{
		template <typename First, typename Second>
		struct default_storage_transformer_t
		{
			using first_type = First;
			using second_type = Second;
		};

		template <typename First, typename Second>
		struct first_as_reference_transformer_t
		{
			using first_type = First&;
			using second_type = Second;
		};

		template <typename First, typename Second,
			template <typename, typename> typename Storage = default_storage_transformer_t,
			bool = std::is_empty_v<First>, bool = std::is_empty_v<Second>>
		struct ebo_pair_tx;

		template <typename First, typename Second, template <typename, typename> typename StorageTransformer>
		struct ebo_pair_tx<First, Second, StorageTransformer, false, false>
		{
			using first_type = typename StorageTransformer<First, Second>::first_type;
			using second_type = typename StorageTransformer<First, Second>::second_type;

			ebo_pair_tx() = default;
			ebo_pair_tx(ebo_pair_tx const&) = default;
			~ebo_pair_tx() = default;

			template <typename F, typename S>
			ebo_pair_tx(F&& first, S&& second)
				: first_(first), second_(second)
			{}

			first_type& first() const { return first_; }
			second_type& second() const { return second_; }

		private:
			first_type first_;
			second_type second_;
		};

		template <typename First, typename Second, template <typename, typename> typename StorageTransformer>
		struct ebo_pair_tx<First, Second, StorageTransformer, true, false>
			: protected First
		{
			using first_type = typename StorageTransformer<First, Second>::first_type;
			using second_type = typename StorageTransformer<First, Second>::second_type;

			ebo_pair_tx() = default;
			ebo_pair_tx(ebo_pair_tx const&) = default;
			~ebo_pair_tx() = default;

			template <typename F, typename S>
			ebo_pair_tx(F&& first, S&& second)
				: First(first), second_(second)
			{}

			first_type& first() { return static_cast<first_type&>(*this); }
			second_type& second() { return second_; }
			first_type const& first() const { return static_cast<first_type const&>(*this); }
			second_type const& second() const { return second_; }

		private:
			second_type second_;
		};

		template <typename First, typename Second, template <typename, typename> typename Tr>
		struct ebo_pair_tx<First, Second, Tr, false, true>
			: protected First
		{
			using first_type = First;
			using second_type = Second;

			ebo_pair_tx() = default;
			ebo_pair_tx(ebo_pair_tx const&) = default;
			~ebo_pair_tx() = default;

			template <typename F, typename S>
			ebo_pair_tx(F&& first, S&& second)
				: Second(second), first_(first)
			{}

			First& first() { return first_; }
			Second& second() { return static_cast<Second&>(*this); }
			First const& first() const { return first_; }
			Second const& second() const { return static_cast<Second const&>(*this); }

		private:
			typename Tr<First, Second>::first_type first_;
		};

		// if they're both zero-size, just inherit from second
		template <typename First, typename Second, template <typename, typename> typename Tr>
		struct ebo_pair_tx<First, Second, Tr, true, true>
			: ebo_pair_tx<First, Second, Tr, false, true>
		{};
	}


	template <typename First, typename Second, template <typename, typename> typename Transformer = detail::default_storage_transformer_t>
	using ebo_pair_t = detail::ebo_pair_tx<First, Second>;
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
//  simple_memory_t
//  -----------------
//
//    this stores a pointer & allocator pair, and allows assignment, indexing, and offsets
//
namespace atma
{
	template <typename T, typename Allocator = std::allocator<T>>
	struct simple_memory_t : detail::base_memory_t<T, Allocator>
	{
		using base_type = detail::base_memory_t<T, Allocator>;

		using allocator_type   = typename base_type::allocator_type;
		using allocator_traits = typename base_type::allocator_traits;
		using value_type       = typename allocator_traits::value_type;
		using pointer          = typename allocator_traits::pointer;
		using const_pointer    = typename allocator_traits::const_pointer;
		using reference        = typename allocator_traits::value_type&;

		simple_memory_t() = default;
		explicit simple_memory_t(allocator_type const&);
		explicit simple_memory_t(pointer, allocator_type const& = allocator_type());
		template <typename B> simple_memory_t(simple_memory_t<T, B> const&);

		auto operator = (simple_memory_t const&) -> simple_memory_t& = default;
		auto operator = (pointer) -> simple_memory_t&;
		template <typename B> auto operator = (simple_memory_t<T, B> const&) -> simple_memory_t&;

		operator bool() const { return ptr_ != nullptr; }
		operator pointer() const { return ptr_; }
		operator const_pointer() const { return ptr_; }

		auto operator *  () const -> reference;
		auto operator [] (intptr) const -> reference;
		auto operator -> () const -> pointer;

		auto data() const -> pointer { return ptr_; }

	protected:
		value_type* ptr_ = nullptr;
	};

	template <typename T>
	simple_memory_t(T*) -> simple_memory_t<T>;

	template <typename T, typename A>
	simple_memory_t(T*, A const&) -> simple_memory_t<T, A>;



	template <typename T, typename A, typename D,
		CONCEPT_MODELS_(atma::concepts::integral, D)>
	inline auto operator + (simple_memory_t<T, A> lhs, D d)
	{
		return simple_memory_t<T, A>(lhs.operator typename simple_memory_t<T, A>::pointer() + d, lhs.allocator());
	}
}

//
//  allocatable_memory_t
//  ----------------------
//
//    this is a memory
//
namespace atma
{
	template <typename T, typename Allocator = std::allocator<T>>
	struct operable_memory_t : simple_memory_t<T, Allocator>
	{
		using base_type = simple_memory_t<T, Allocator>;

		using allocator_type   = typename base_type::allocator_type;
		using allocator_traits = typename base_type::allocator_traits;
		using value_type       = typename allocator_traits::value_type;
		using pointer          = typename allocator_traits::pointer;
		using const_pointer    = typename allocator_traits::const_pointer;
		using reference        = typename allocator_traits::value_type&;

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
	};
}

//
//  allocatable_memory_t
//  ----------------------
//
//    this is a memory
//
namespace atma
{
	template <typename T, typename Allocator = atma::aligned_allocator_t<T>>
	struct allocatable_memory_t : operable_memory_t<T, Allocator>
	{
		using base_type = operable_memory_t<T, Allocator>;

		using allocator_type   = typename base_type::allocator_type;
		using allocator_traits = typename base_type::allocator_traits;
		using value_type       = typename allocator_traits::value_type;
		using pointer          = typename allocator_traits::pointer;
		using const_pointer    = typename allocator_traits::const_pointer;
		using reference        = typename allocator_traits::value_type&;

		// use base's constructors
		using base_type::base_type;

		allocatable_memory_t(memory_allocate_copy_tag, const_pointer, size_t size, allocator_type const& = allocator_type());
		explicit allocatable_memory_t(size_t capacity, allocator_type const& = allocator_type());

		auto operator = (allocatable_memory_t const&) -> allocatable_memory_t& = default;
		auto operator = (pointer) -> allocatable_memory_t&;
		template <typename B> auto operator = (allocatable_memory_t<T, B> const&) -> allocatable_memory_t&;


		// allocator interface
		auto allocate(size_t) -> bool;
		auto deallocate(size_t = 0) -> void;
	};
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
	using basic_memory_t = allocatable_memory_t<T, Allocator>;

	using memory_t = basic_memory_t<byte>;
}




namespace atma
{
	template <typename T, typename A>
	inline simple_memory_t<T, A>::simple_memory_t(allocator_type const& allocator)
		: base_type(allocator)
	{}

	template <typename T, typename A>
	inline simple_memory_t<T, A>::simple_memory_t(pointer data, allocator_type const& alloc)
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
	inline auto simple_memory_t<T, A>::operator = (pointer rhs) -> simple_memory_t &
	{
		ptr_ = rhs;
		return *this;
	}

	template <typename T, typename A>
	template <typename B>
	inline auto simple_memory_t<T, A>::operator = (simple_memory_t<T, B> const& rhs) -> simple_memory_t &
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
	template <typename T, typename A>
	template <typename... Args>
	inline auto operable_memory_t<T, A>::construct(size_t idx, Args&&... args) -> void
	{
		allocator_traits::construct(this->allocator(), this->ptr_ + idx, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	template <typename... Args>
	inline auto operable_memory_t<T, A>::construct_range(size_t idx, size_t count, Args&&... args) -> void
	{
		for (auto px = this->ptr_ + idx; count--> 0; ++px)
			allocator_traits::construct(this->allocator(), px, std::forward<Args>(args)...);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::copy_construct_range(size_t idx, value_type const* src, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx; count --> 0; ++px, ++src)
			allocator_traits::construct(this->allocator(), px, *src);
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::move_construct_range(size_t idx, value_type* src, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx, py = src; count--> 0; ++px, ++py)
			allocator_traits::construct(this->allocator(), px, std::move(*py));
	}

	template <typename T, typename A>
	inline auto operable_memory_t<T, A>::destruct(size_t idx, size_t count) -> void
	{
		for (auto px = this->ptr_ + idx; count--> 0; ++px)
			allocator_traits::destroy(this->allocator(), px);
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
			allocator_traits::construct(this->allocator(), px, *begin);
	}

	template <typename T, typename A>
	template <typename H>
	inline auto operable_memory_t<T, A>::move_construct_range(size_t idx, H begin, H end) -> void
	{
		for (auto px = this->ptr_ + idx; begin != end; ++begin, ++px)
			allocator_traits::construct(this->allocator(), px, std::move(*begin));
	}

	//template <typename T, typename A>
	//inline auto operator + (operable_memory_t<T, A> const& lhs, typename operable_memory_t<T, A>::allocator_type::difference_type x) -> typename operable_memory_t<T, A>::value_type*
	//{
	//	return static_cast<typename operable_memory_t<T, A>::value_type*>(lhs) + x;
	//}
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

	template <typename T, typename A>
	inline auto allocatable_memory_t<T, A>::allocate(size_t count) -> bool
	{
		this->ptr_= allocator_traits::allocate(this->allocator(), count);
		return this->ptr_ != nullptr;
	}

	template <typename T, typename A>
	inline auto allocatable_memory_t<T, A>::deallocate(size_t count) -> void
	{
		allocator_traits::deallocate(this->allocator(), this->ptr_, count);
	}

}




// anything that inherits from simple_memory_t is considered a memory type
namespace atma
{
	struct memory_span_concept
		: concepts::refines<random_access_range_concept>
	{
		template <typename Memory>
		auto contract() -> concepts::specifies<
			SPECIFIES_EXPR(std::declval<Memory>().allocator()),
			SPECIFIES_EXPR(std::declval<Memory>().data())
		>;
	};

	template <typename, typename = std::void_t<>>
	struct is_memory_type_t : std::false_type {};


	template <typename M>
	struct is_memory_type_t<M, std::void_t<typename M::value_type, typename M::allocator_type>>
		: std::bool_constant<std::is_base_of<simple_memory_t<typename M::value_type, typename M::allocator_type>, M>::value>
	{};

	template <typename M>
	constexpr bool is_memory_type_v = is_memory_type_t<M>::value;
}


// memxfer_range_t
namespace atma
{
	constexpr struct memory_dest_tag_t {} memory_dest_tag;
	constexpr struct memory_src_tag_t {} memory_src_tag;

	template <typename Iter>
	using enable_if_contiguous_iterator_t =
		std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename std::iterator_traits<Iter>::iterator_category>>;

	// a range that can provide begin/end
	template <typename Range>
	using enable_if_contiguous_range_t =
		std::enable_if_t<atma::concepts::models_v<random_access_range_concept, Range>>;

	// a range that can only provide 'begin' (maybe 'data')
	template <typename Range>
	using enable_if_baseaddr_range_t =
		void;

	template <typename Tag, typename T, typename A>
	struct memxfer_range_t
	{
		using allocator_type = A;

		memxfer_range_t(T* ptr, size_t idx, size_t size)
			: alloc_and_ptr_(allocator_type(), ptr + idx)
			, size_(size)
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		memxfer_range_t(Range const& range)
			: alloc_and_ptr_(allocator_type(), &*begin(range))
			, size_(std::distance(std::begin(range), std::end(range)))
		{}

		template <typename Range, CONCEPT_MODELS_(random_access_range_concept, Range)>
		memxfer_range_t(Range const& range, size_t idx, size_t size)
			: alloc_and_ptr_(allocator_type(), &range[idx])
			, size_(size)
		{}

		template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>> //CONCEPT_MODELS_(memory_span_concept, M)>
		memxfer_range_t(M memory, size_t size)
			: alloc_and_ptr_(memory.allocator(), memory.data())
			, size_(size)
		{}

		template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>> //CONCEPT_MODELS_(memory_span_concept, M)>
		memxfer_range_t(M memory, size_t idx, size_t size)
			: alloc_and_ptr_(memory.allocator(), memory.data() + idx)
			, size_(size)
		{}

		template <typename Iter> // typename = enable_if_contiguous_iterator_t<Iter>
		memxfer_range_t(Iter begin, Iter end)
			: alloc_and_ptr_(allocator_type(), &*begin)
			, size_(std::distance(begin, end))
		{}

		memxfer_range_t(std::vector<std::remove_const_t<T>> const& range)
			: alloc_and_ptr_(allocator_type(), range.data())
			, size_(range.size())
		{}

		~memxfer_range_t()
		{}

		allocator_type& allocator() const { return const_cast<allocator_type&>(alloc_and_ptr_.first()); }
		T* begin() { return alloc_and_ptr_.second(); }
		T* end() { return alloc_and_ptr_.second() + size_; }
		T const* begin() const { return alloc_and_ptr_.second(); }
		T const* end() const { return alloc_and_ptr_.second() + size_; }
		size_t size() const { return size_; }

	private:
		// store the allocator EBO-d with the vtable pointer
		using alloc_and_ptr_type = ebo_pair_t<A, T*, detail::first_as_reference_transformer_t>;
		alloc_and_ptr_type alloc_and_ptr_;
		size_t size_ = 0;
	};

	// ranged-based-for
	template <typename G, typename T, typename A>
	inline auto begin(memxfer_range_t<G, T, A>& r)
		{ return r.begin(); }

	template <typename G, typename T, typename A>
	inline auto end(memxfer_range_t<G, T, A>& r)
		{ return r.end(); }

	template <typename G, typename T, typename A>
	inline auto begin(memxfer_range_t<G, T, A> const& r)
		{ return r.begin(); }

	template <typename G, typename T, typename A>
	inline auto end(memxfer_range_t<G, T, A> const& r)
		{ return r.end(); }
}

// dest_range
namespace atma
{
	// dest/src versions
	template <typename T, typename A>
	struct dest_range_t : memxfer_range_t<memory_dest_tag_t, T, A>
	{
		using memxfer_range_t<memory_dest_tag_t, T, A>::memxfer_range_t;
	};

	// deduction guides
	template <typename T>
	dest_range_t(T*, size_t, size_t) -> dest_range_t<T, std::allocator<T>>;

	template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>>
	dest_range_t(M, size_t, size_t) -> dest_range_t<typename M::value_type, typename M::allocator_type>;

	template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>>
	dest_range_t(M, size_t) -> dest_range_t<typename M::value_type, typename M::allocator_type>;
}

// src_range
namespace atma
{
	template <typename T, typename A>
	struct src_range_t : memxfer_range_t<memory_src_tag_t, T, A>
	{
		using memxfer_range_t<memory_src_tag_t, T, A>::memxfer_range_t;
	};

	template <typename T>
	src_range_t(T*, size_t, size_t) -> src_range_t<T, std::allocator<T>>;

	template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>>
	src_range_t(M, size_t, size_t) -> src_range_t<typename M::value_type, typename M::allocator_type>;

	template <typename M, typename = std::enable_if_t<is_memory_type_v<M>>>
	src_range_t(M, size_t) -> src_range_t<typename M::value_type, typename M::allocator_type>;

	template <typename T>
	src_range_t(std::vector<T> const&) -> src_range_t<T const, std::allocator<T>>;

	template <typename I, CONCEPT_MODELS_(concepts::contiguous_iterator_concept, I)>    /// typename = enable_if_contiguous_iterator_t<I>, typename T = std::remove_reference_t<decltype(*begin(std::declval<I>()))>>
	src_range_t(I begin, I end) -> src_range_t
		<int, std::allocator<int>>;
		//< std::remove_reference_t<decltype(*begin(std::declval<I>()))>
		//, std::allocator<std::remove_reference_t<decltype(*begin(std::declval<I>()))>>
		//>;
}



// construct
namespace atma::memory
{
	template <typename T, typename A, typename... Args>
	inline auto construct(simple_memory_t<T, A>& ptr, Args&&... args) -> void
	{
		simple_memory_t<T, A>::allocator_traits::construct(ptr.allocator(), (T*)ptr, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	inline auto construct(T* ptr, Args&&... args) -> void
	{
		new (ptr) T(std::forward<Args>(args)...);
	}
}

// construct_at
namespace atma::memory
{
	template <typename T, typename A, typename... Args>
	inline auto construct_at(simple_memory_t<T, A>& ptr, size_t idx, Args&&... args) -> void
	{
		simple_memory_t<T, A>::allocator_traits::construct(ptr.allocator(), &ptr[idx], std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	inline auto construct_at(T* ptr, size_t idx, Args&&... args) -> void
	{
		new (ptr + idx) T(std::forward<Args>(args)...);
	}
}

// construct_range
namespace atma::memory
{
	template <typename T, typename A, typename... Args>
	inline auto construct_range(dest_range_t<T, A> range, Args&&... args) -> void
	{
		using dest_alloc_traits = std::allocator_traits<A>;
		for (auto& x : range)
			dest_alloc_traits::construct(range.allocator(), &x, std::forward<Args>(args)...);
	}
}

// copy_construct_range
namespace atma::memory
{
	template <typename DT, typename DA, typename ST, typename SA>
	inline auto copy_construct_range(dest_range_t<DT, DA> dest_range, src_range_t<ST, SA> src_range) -> void
	{
		ATMA_ASSERT(dest_range.size() == src_range.size());

		DT* px = begin(dest_range);
		auto pxe = end(dest_range);
		auto py = begin(src_range);

		using dest_alloc_traits = std::allocator_traits<DA>;
		while (px != pxe)
			dest_alloc_traits::construct(dest_range.allocator(), &*px++, *py++);
	}
}

// move_construct_range
namespace atma::memory
{
	template <typename T, typename A, typename B>
	inline auto move_construct_range(dest_range_t<T, A> dest_range, src_range_t<T, B> src_range) -> void
	{
		ATMA_ASSERT(dest_range.size() == src_range.size());

		auto px = (T*)dest_range.ptr + dest_range.idx;
		auto py = (T*)src_range.ptr + src_range.idx;

		using dest_alloc_traits = std::allocator_traits<A>;
		while (dest_range.size --> 0 && src_range.size --> 0)
			dest_alloc_traits::construct(dest_range.allocator, px++, std::move(*py++));
	}

	template <typename T, typename A, typename SrcIt>
	inline auto move_construct_range(dest_range_t<T, A> dest_range, SrcIt src_begin, SrcIt src_end) -> void
	{
		auto px = (T*)dest_range.ptr + dest_range.idx;

		using dest_alloc_traits = std::allocator_traits<A>;
		while (dest_range.size--> 0 && src_begin != src_end)
			dest_alloc_traits::construct(dest_range.allocator, px++, std::move(*src_begin++));
	}
}