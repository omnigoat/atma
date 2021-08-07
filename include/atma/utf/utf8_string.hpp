#pragma once

#include <atma/utf/utf8_string_range_header.hpp>
#include <atma/assert.hpp>
#include <type_traits>
#include <span>


//=====================================================================
//
//  concepts in utf8 string handling
//  ==================================
//
//  utf8 characters are a varying-length number of bytes. this means
//  that operations that modify the character-code can change the
//  number of bytes required for storage. types that only point to one
//  character at a time (utf8_char/iterator) don't have enough info
//  as to what to do in these situations. imagine pointing to one char
//  in a string - without knowing the bounds of the string, you can't
//  modify the character as you may need to shuffle all subsequent
//  bytes "up".
//
//  types that deal with a single character, like a utf8_char or
//  an iterator, point to immutable data. types that represent a range
//  of characters, like a span or a string, can point to either mutable
//  or immutable data, as routines that edit the data will have enough
//  information to make informed decisions.
//
//=====================================================================
namespace atma
{
	struct utf8_char_t;
	//struct utf8_iterator_t;
	struct utf8_string_t;
}


//=====================================================================
//
//  overview of terminology
//  =========================
//
//    utf8_byte_...
//    ---------------
//      the utf8_byte prefix relates to bytes in a utf8 string. any
//      bytes. this is different from chars.
//
//    utf8_char_...
//    ---------------
//      utf8_char prefixes anything that deals with a sequence of bytes
//      that make up a multibyte utf8 codepoint. it is assumed that
//      any utf8_char function is operating on data aligned to a
//      *leading* utf8 byte. a.k.a: at the beginning of a sequence.
//
//      sometimes these functions accept a single char. whatever.
//
//    utf8_charseq_...
//    ------------------
//      utf8_charseq signifies a sequence of utf8-chars, basically a
//      raw-string. charseqs are used for algorithms, mainly.
//
//=====================================================================
namespace atma::detail
{
	constexpr int const char_length_table[] =
	{
		// ascii
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

		// run-on bytes. zero I guess.
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

		// first two are invalid, rest are two-byte
		-1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

		// three byte!
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

		// four byte!
		4, 4, 4, 4, 4,

		// invalid!
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};
}


//=====================================================================
// utf8-byte functions
//=====================================================================
namespace atma
{
	constexpr auto utf8_byte_is_valid(byte const) -> bool;
	constexpr auto utf8_byte_is_leading(byte const) -> bool;
	constexpr auto utf8_byte_is_run_on(byte const) -> bool;
	constexpr auto utf8_byte_length(byte const) -> int;
}

//=====================================================================
// utf8-char functions
//=====================================================================
namespace atma
{
	constexpr auto utf8_char_is_ascii(char const c) -> bool;
	constexpr auto utf8_char_is_newline(char const* leading) -> bool;
	constexpr auto utf8_char_size_bytes(char const* leading) -> int;
	constexpr auto utf8_char_equality(char const* lhs, char const* rhs) -> bool;

	constexpr auto utf8_char_advance(char const*& leading) -> void;
}

//=====================================================================
//
//  @utf8-charseq functions
//  ----------------------------
//  operations on a sequence of utf8-chars. "char const*" is
//  indistinguishable when used as both a single char or many
//
namespace atma
{
	template <typename FN>
	auto utf8_charseq_for_each(char const* seq, FN&& fn) -> void;

	template <typename T>
	auto utf8_charseq_any_of(char const* seq, T&& pred) -> bool;

	auto utf8_charseq_find(char const* seq, utf8_char_t x) -> char const*;

	// returns how many bytes we traversed to get to char-idx
	auto utf8_charseq_idx_to_byte_idx(char const* seq, size_t sz, size_t char_idx) -> size_t;
}


//=====================================================================
//
//  CONCEPTS
//
namespace atma
{
	// forward-range of utf8_char_t
	template <typename R>
	concept utf8_range_concept =
		std::ranges::forward_range<R> &&
		std::is_convertible_v<std::ranges::range_value_t<R>, utf8_char_t>;

	// contiguous-range of chars
	template <typename BackingType, typename R>
	concept utf8_charseq_range_concept =
		std::ranges::contiguous_range<R> &&
		std::is_convertible_v<std::ranges::range_value_t<R>, BackingType>;

	// std::data + .size_bytes  =>  acceptable
	template <typename R, typename ElementType>
	concept utf8_data_size_bytes_concept =
		requires (R const& r)
		{
			{ r.size_bytes() };
			{ std::data(r) } -> std::convertible_to<ElementType*>;
		};

	// a forward-iterator that dereferences to a utf8_char_t
	template <typename It>
	concept utf8_iterator_concept =
		std::forward_iterator<It> &&
		std::is_convertible_v<std::iter_value_t<It>, utf8_char_t>;

	// a contiguous-iterator that can be turned into a char const*
	template <typename It>
	concept utf8_charseq_iterator_concept =
		std::contiguous_iterator<It> &&
		requires(It it)
		{
			{ std::to_address(it) } -> std::convertible_to<char const*>;
		};

	// sometimes we pass iterators to algorithms that later make the distinction
	template <typename It>
	concept utf8_fwding_iterator_concept =
		utf8_iterator_concept<It> || utf8_charseq_iterator_concept;

	template <typename F>
	concept utf8_char_fn_concept = std::is_invocable_v<F, utf8_char_t>;
}


//=====================================================================
//
//  @utf8_char_t
//  ---------------
//  
//
namespace atma
{
	struct utf8_char_t
	{
		constexpr utf8_char_t() = default;
		constexpr utf8_char_t(utf8_char_t const&) = default;

		constexpr explicit utf8_char_t(char);
		constexpr explicit utf8_char_t(char const*);

		auto as_bytes() const { return std::span<byte, 4>((byte*)bytes_, 4); }

		constexpr auto operator [](size_t idx) const -> byte
		{
			ATMA_ASSERT(idx < size_bytes());
			return bytes_[idx];
		}

		auto data() const -> char const* { return reinterpret_cast<char const*>(bytes_); }
		constexpr auto size_bytes() const -> size_t { return utf8_byte_length(bytes_[0]); }

		explicit operator char const* () const { return data(); }

		auto operator = (utf8_char_t const&) -> utf8_char_t& = default;

	private:
		byte bytes_[4] = {byte(), byte(), byte(), byte()};
	};

}


namespace atma
{
	inline auto utf8_char_is_newline(utf8_char_t const& x) -> bool
	{
		auto const c = (char)x[0];
		return c == 0x0a || c == 0x0b || c == 0x0c || c == 0x0d || c == 0x85;
	}
}


//=====================================================================
//
//  @utf8_span_t
//  ---------------
//  a utf8 span that is really just a span of char
//
//
namespace atma::detail
{
	template <typename T>
	struct basic_utf8_span_t
	{
		// adhere to standard with all the types
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = T const*;
		using reference = T&;
		using const_reference = T const&;
		using iterator = value_type*;
		using reverse_iterator = std::reverse_iterator<iterator>;

		constexpr basic_utf8_span_t() = default;
		constexpr basic_utf8_span_t(basic_utf8_span_t const&) = default;

		constexpr basic_utf8_span_t(value_type* data, size_t size)
			: data_(data)
			, size_(size)
		{}

		// construct from a range of compatible values
		template <utf8_charseq_range_concept<T> Range>
		constexpr basic_utf8_span_t(Range const& range)
			: data_(std::data(range))
			, size_(std::size(range))
		{}

		// if something is not a contiguous-range but provides data & size_bytes accessors
		// then we will assume it is a contiguous byte-array "underneath"
		template <utf8_data_size_bytes_concept<value_type> Range>
		requires !utf8_charseq_range_concept<T, Range>
		constexpr basic_utf8_span_t(Range const& range)
			: data_(std::data(range))
			, size_(range.size_bytes())
		{}

		// construct from a pair of contiguous iterators
		template <utf8_charseq_iterator_concept Iterator>
		constexpr basic_utf8_span_t(Iterator begin, Iterator end)
			: data_(std::to_address(begin))
			, size_(end - begin)
		{}

		auto operator = (basic_utf8_span_t const& rhs) -> basic_utf8_span_t& = default;

		// iterators
		constexpr auto begin() const { return data_; }
		constexpr auto end() const { return data_ + size_; }
		constexpr auto rbegin() const { return reverse_iterator(end()); }
		constexpr auto rend() const { return reverse_iterator(begin()); }

		// access
		constexpr auto data() const { return data_; }
		constexpr auto front() const { return *data_; }
		constexpr auto back() const { return data_[size_ - 1]; }
		constexpr auto operator [](size_type idx) const { return data_[idx]; }

		// observers
		constexpr auto size() const { return size_; }
		constexpr auto size_bytes() const { return size_; }
		constexpr auto empty() const { return size_ == 0; }

		// subviews
		constexpr auto first(size_type count) const { return basic_utf8_span_t(data_, count); }
		constexpr auto last(size_type count) const { return basic_utf8_span_t(data_ + size_ - count, count); }
		constexpr auto subspan(size_type offset, size_type count) const { return basic_utf8_span_t(data_ + offset, count); }

	private:
		value_type* data_ = nullptr;
		size_type size_ = 0;
	};
}

namespace atma
{
	using utf8_span_t       = detail::basic_utf8_span_t<char>;
	using utf8_const_span_t = detail::basic_utf8_span_t<char const>;
}


//=====================================================================
//
//  @utf8_iterator_t
//  -------------------
//  given a sequence of chars, iterates character-by-character. this
//  means that the amount of bytes traversed is variable, as utf8 is
//  want to be
//
namespace atma::detail
{
	struct basic_utf8_iterator_t
	{
		using value_type = utf8_char_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type;
		using pointer = value_type*;
		using iterator_category = std::forward_iterator_tag;

		basic_utf8_iterator_t() = default;
		basic_utf8_iterator_t(basic_utf8_iterator_t const&) = default;
		basic_utf8_iterator_t(char const*);

		// access
		auto operator *() const { return value_type{here_}; }
		auto operator ->() const { return value_type{here_}; }

		// travel
		auto operator ++() -> basic_utf8_iterator_t&;
		auto operator ++(int) -> basic_utf8_iterator_t;
		auto operator --() -> basic_utf8_iterator_t&;
		auto operator --(int) -> basic_utf8_iterator_t;

		auto char_data() const { return here_; }

	private:
		char const* here_ = nullptr;
	};

	static_assert(std::is_convertible_v<std::iter_value_t<basic_utf8_iterator_t>, utf8_char_t>);
}

namespace atma
{
	using utf8_iterator_t = detail::basic_utf8_iterator_t;
}


//=====================================================================
//
//  @basic_utf8_range_t
//  ---------------------------
//  A string-range that does not allocate any memory, but instead points
//  to external, contiguous memory that is expected to be valid for the
//  lifetime of the string-range.
//
//  The primary difference between utf8_span... and utf8_range... is
//  that utf8_range is logically a sequence of utf8_chars, whereas the
//  utf8_span is just a sequence of plain-old chars.
//
//  This range is immutable.
//
//=====================================================================
namespace atma::detail
{
	template <typename BackingType>
	struct basic_utf8_range_t
	{
		static_assert(std::is_same_v<BackingType, char> || std::is_same_v<BackingType, char const>);

		using backing_type = BackingType;
		using value_type = transfer_const_t<backing_type, utf8_char_t>;
		using iterator = utf8_iterator_t;

		basic_utf8_range_t();
		basic_utf8_range_t(basic_utf8_range_t const&);

		// construct from a range of compatible values
		template <utf8_charseq_range_concept<BackingType> Range>
		explicit basic_utf8_range_t(Range const&);

		// conversion from iterator-pairs
		template <utf8_charseq_iterator_concept IteratorType>
		basic_utf8_range_t(IteratorType, IteratorType);

		auto size_bytes() const -> size_t;
		auto empty() const -> bool;

		auto begin() const -> iterator;
		auto end() const -> iterator;

	private:
		backing_type* const begin_ = nullptr;
		backing_type* const end_ = nullptr;
	};
}

namespace atma
{
	using utf8_range_t       = detail::basic_utf8_range_t<char>;
	using utf8_const_range_t = detail::basic_utf8_range_t<char const>;
}





//=====================================================================
//
//  @utf8_string_t
//  -----------------
//  conceptually this is a forward-range of utf8_char_ts.
//  implementation-wise, it is a standard char-based string. the access
//  to its underlying representation is something of an open question
//  at the moment.
//
//=====================================================================
namespace atma
{
	struct utf8_string_t
	{
		using value_type = utf8_char_t;
		using size_type = size_t;
		using difference_type = ptrdiff_t;
		using iterator = utf8_iterator_t;
		using const_iterator = utf8_iterator_t;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr utf8_string_t() = default;
		utf8_string_t(char const* str, size_t size);
		utf8_string_t(char const* str_begin, char const* str_end);
		utf8_string_t(char const* str);
		utf8_string_t(const_iterator const&, const_iterator const&);

		// construct from any char-based range
		template <utf8_charseq_range_concept<char const> Range>
		explicit utf8_string_t(Range const&);

		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		~utf8_string_t();

		auto operator = (utf8_string_t const&) -> utf8_string_t&;
		auto operator = (utf8_string_t&&) -> utf8_string_t&;
		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&;
		auto operator += (char const*) -> utf8_string_t&;

		// element access
		auto c_str() const -> char const*;
		auto data() const { return data_; }

		// observers
		auto size_bytes() const -> size_t;
		[[nodiscard]] auto empty() const -> bool;

		// iterators
		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto rbegin() const -> const_reverse_iterator;
		auto rend() const -> const_reverse_iterator;

		auto raw_begin() const -> char const*;
		auto raw_end() const -> char const*;

		// operations
		auto push_back(char) -> void;
		auto push_back(utf8_char_t) -> void;

		auto append(char const*, char const*) -> utf8_string_t&;
		auto append(char const*) -> utf8_string_t&;
		auto append(utf8_string_t const&) -> utf8_string_t&;
		auto append(utf8_const_span_t const&) -> utf8_string_t&;

		auto clear() -> void;

	private:
		auto imem_quantize(size_t, bool keep) -> void;
		auto imem_quantize_grow(size_t) -> void;

		auto imem_quantize_capacity(size_t) const -> size_t;
		auto imem_realloc(size_t, bool keep) -> void;


	private:
		size_t capacity_ = 0;
		size_t size_ = 0;
		char* data_ = nullptr;

		friend class utf16_string_t;
	};


	auto operator == (utf8_string_t const&, utf8_string_t const&) -> bool;
	auto operator == (utf8_string_t const&, char const*) -> bool;
	auto operator == (char const*, utf8_string_t const&) -> bool;

	auto operator != (utf8_string_t const&, utf8_string_t const&) -> bool;
	auto operator != (utf8_string_t const&, char const*) -> bool;
	auto operator != (char const*, utf8_string_t const&) -> bool;

	auto operator + (utf8_string_t const&, utf8_string_t const&)       -> utf8_string_t;
	auto operator + (utf8_string_t const&, char const*)                -> utf8_string_t;
	auto operator + (utf8_string_t const&, std::string const&)         -> utf8_string_t;
	auto operator + (utf8_string_t const&, utf8_const_span_t const&)   -> utf8_string_t;

	auto operator << (std::ostream&, utf8_string_t const&) -> std::ostream&;
}




//=====================================================================
// @utf8_char_t implementation
//=====================================================================
namespace atma
{
	constexpr utf8_char_t::utf8_char_t(char c)
		: bytes_{byte(c), byte(), byte(), byte()}
	{
		ATMA_ASSERT(utf8_char_is_ascii(c));
	}

	constexpr utf8_char_t::utf8_char_t(char const* c)
	{
		if (c == nullptr)
			return;

		uint idx = 0;
		switch (detail::char_length_table[*c])
		{
			case 4: bytes_[idx++] = (byte)*c++; [[fallthrough]];
			case 3: bytes_[idx++] = (byte)*c++; [[fallthrough]];
			case 2: bytes_[idx++] = (byte)*c++; [[fallthrough]];
			case 1: bytes_[idx++] = (byte)*c++;
				break;

			default:
				ATMA_HALT("bad leading-byte of a utf8-char");
				break;
		}
	}

	inline auto operator == (utf8_char_t lhs, utf8_char_t rhs) -> bool
	{
		return utf8_char_equality(lhs.data(), rhs.data());
	}

	inline auto operator == (utf8_char_t lhs, char const* rhs) -> bool
	{
		return utf8_char_equality(lhs.data(), rhs);
	}

	inline auto operator == (utf8_char_t lhs, char rhs) -> bool
	{
		ATMA_ASSERT(utf8_char_is_ascii(rhs));
		return lhs[0] == byte(rhs);
	}

	inline auto operator != (utf8_char_t lhs, char rhs) -> bool
	{
		ATMA_ASSERT(utf8_char_is_ascii(rhs));
		return lhs[0] != byte(rhs);
	}
}


//=====================================================================
// @basic_utf8_iterator_t implementation
//=====================================================================
namespace atma::detail
{
	inline basic_utf8_iterator_t::basic_utf8_iterator_t(char const* here)
		: here_(here)
	{}

	inline auto basic_utf8_iterator_t::operator++ () -> basic_utf8_iterator_t&
	{
		here_ += utf8_char_size_bytes(here_);
		return *this;
	}

	inline auto basic_utf8_iterator_t::operator++ (int) -> basic_utf8_iterator_t
	{
		auto r = *this;
		++r;
		return r;
	}

	inline auto basic_utf8_iterator_t::operator-- () -> basic_utf8_iterator_t&
	{
		while (!utf8_byte_is_leading(byte(*--here_)));
		return *this;
	}

	inline auto basic_utf8_iterator_t::operator-- (int) -> basic_utf8_iterator_t
	{
		auto r = *this;
		--r;
		return r;
	}

	inline auto operator == (basic_utf8_iterator_t const& lhs, basic_utf8_iterator_t const& rhs)
	{
		return lhs.char_data() == rhs.char_data();
	}

	inline auto operator != (basic_utf8_iterator_t const& lhs, basic_utf8_iterator_t const& rhs)
	{
		return !operator == (lhs, rhs);
	}
}


//=====================================================================
// @utf8_span_t implementation
//=====================================================================
namespace atma::detail
{
	template <typename BT>
	constexpr auto operator == (basic_utf8_span_t<BT> lhs, basic_utf8_span_t<BT> rhs) -> bool
	{
		return lhs.data() == rhs.data() && lhs.size() == rhs.size();
	}

	template <typename BT>
	constexpr auto operator != (basic_utf8_span_t<BT> lhs, basic_utf8_span_t<BT> rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}
}


//=====================================================================
// @basic_utf8_range_t implementation
//=====================================================================
namespace atma::detail
{
	template <typename BT>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t()
		: begin_(), end_()
	{}

	template <typename BT>
	template <utf8_charseq_range_concept<BT> Range>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(Range const& range)
	{
	}

	template <typename BT>
	template <utf8_charseq_iterator_concept IteratorType>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(IteratorType begin, IteratorType end)
		: begin_(std::to_address(begin)), end_(std::to_address(end))
	{}

	template <typename BT>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(basic_utf8_range_t const& rhs)
		: begin_(rhs.begin_), end_(rhs.end_)
	{}

	template <typename BT>
	inline auto basic_utf8_range_t<BT>::size_bytes() const -> size_t
	{
		return end_ - begin_;
	}

	template <typename BT>
	inline auto basic_utf8_range_t<BT>::empty() const -> bool
	{
		return begin_ == end_;
	}

	template <typename BT>
	inline auto basic_utf8_range_t<BT>::begin() const -> iterator
	{
		return begin_;
	}

	template <typename BT>
	inline auto basic_utf8_range_t<BT>::end() const -> iterator
	{
		return end_;
	}
}

//=====================================================================
// !basic_utf8_range_t operators
//=====================================================================
namespace atma::detail
{
#if 0
	template <typename BT>
	inline auto operator == (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return lhs.size_bytes() == rhs.size_bytes() && memcmp(lhs.begin(), rhs.begin(), lhs.size_bytes()) == 0;
	}

	template <typename BT>
	inline auto operator != (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	template <typename BT>
	inline auto operator == (basic_utf8_range_t<BT> const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.size_bytes() == rhs.size_bytes() && memcmp(lhs.begin(), rhs.raw_begin(), lhs.size_bytes()) == 0;
	}

	template <typename BT>
	inline auto operator == (utf8_string_t const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return lhs.size_bytes() == rhs.size_bytes() && memcmp(lhs.raw_begin(), rhs.begin(), lhs.size_bytes()) == 0;
	}

	template <typename BT>
	inline auto operator != (basic_utf8_range_t<BT> const& lhs, utf8_string_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	template <typename BT>
	inline auto operator != (utf8_string_t const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	template <typename BT>
	inline auto operator == (basic_utf8_range_t<BT> const& lhs, char const* rhs) -> bool
	{
		return strncmp(lhs.begin(), rhs, lhs.size_bytes()) == 0;
	}

	template <typename BT>
	inline auto operator < (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		auto cmp = ::strncmp(
			lhs.begin(), rhs.begin(),
			std::min(lhs.size_bytes(), rhs.size_bytes()));

		return
			cmp < 0 || (!(0 < cmp) && (
				lhs.size_bytes() < rhs.size_bytes()));
	}
#endif

	template <typename BT>
	inline auto operator << (std::ostream& stream, basic_utf8_range_t<BT> const& xs) -> std::ostream&
	{
		for (auto x : xs)
			stream.put(x);
		return stream;
	}




	//=====================================================================
	// functions
	//=====================================================================
	template <typename BT>
	inline auto strncmp(basic_utf8_range_t<BT> const& lhs, char const* str, uint32 n) -> uint32
	{
		return std::strncmp(lhs.begin(), str, n);
	}

	template <typename BT>
	inline auto rebase_string_range(utf8_string_t const& rebase, utf8_string_t const& oldbase, basic_utf8_range_t<BT> const& range) -> basic_utf8_range_t<BT>
	{
		return{
			rebase.raw_begin() + (range.begin() - oldbase.raw_begin()),
			rebase.raw_begin() + (range.end() - oldbase.raw_begin())
		};
	}
}


//=====================================================================
// @utf8_string_t implementation
//=====================================================================
namespace atma
{
	inline utf8_string_t::utf8_string_t(char const* str, size_t size)
		: capacity_(imem_quantize_capacity(size))
		, size_(size)
		, data_(new char[capacity_])
	{
		memcpy(data_, str, size_);
		data_[size_] = '\0';
	}

	inline utf8_string_t::utf8_string_t(char const* begin, char const* end)
		: utf8_string_t(begin, end - begin)
	{}

	inline utf8_string_t::utf8_string_t(char const* str)
		: utf8_string_t(str, strlen(str))
	{}

	template <utf8_charseq_range_concept<char const> Range>
	inline utf8_string_t::utf8_string_t(Range const& range)
		: utf8_string_t(std::data(range), std::size(range))
	{}

	inline utf8_string_t::utf8_string_t(const_iterator const& begin, const_iterator const& end)
		: utf8_string_t(begin.char_data(), end.char_data() - begin.char_data())
	{}

	inline utf8_string_t::utf8_string_t(utf8_string_t const& str)
		: utf8_string_t(str.raw_begin(), str.size_bytes())
	{}

#if 0
	utf8_string_t::utf8_string_t(const utf16_string_t& rhs)
	{
		// optimistically reserve one char per char16_t. this will work optimally
		// for english text, and will still totes work fine for anything else.
		chars_.reserve(rhs.char_count_);

		utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
	}
#endif

	inline utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
		: capacity_(rhs.capacity_)
		, size_(rhs.size_)
		, data_(rhs.data_)
	{
		rhs.capacity_ = 0;
		rhs.size_ = 0;
		rhs.data_ = nullptr;
	}

	inline utf8_string_t::~utf8_string_t()
	{
		delete [] data_;
		capacity_ = 0;
		size_ = 0;
		data_ = nullptr;
	}

	inline auto utf8_string_t::operator += (utf8_string_t const& rhs) -> utf8_string_t&
	{
		imem_quantize(size_ + rhs.size_, true);

		memcpy(data_ + size_, rhs.data_, rhs.size_);
		size_ += rhs.size_;
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::operator += (char const* rhs) -> utf8_string_t&
	{
		auto rhslen = strlen(rhs);
		imem_quantize(size_ + rhslen, true);

		memcpy(data_ + size_, rhs, rhslen);
		size_ += rhslen;
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::operator = (utf8_string_t const& rhs) -> utf8_string_t&
	{
		imem_quantize(rhs.size_, false);

		size_ = rhs.size_;
		memcpy(data_, rhs.data_, size_);
		data_[size_] = '\0';

		return *this;
	}

	inline auto utf8_string_t::operator = (utf8_string_t&& rhs) -> utf8_string_t&
	{
		std::swap(capacity_, rhs.capacity_);
		std::swap(size_, rhs.size_);
		std::swap(data_, rhs.data_);
		return *this;
	}

	inline auto utf8_string_t::empty() const -> bool
	{
		return size_ == 0;
	}

	inline auto utf8_string_t::size_bytes() const -> size_t
	{
		return size_;
	}

	inline auto utf8_string_t::c_str() const -> char const*
	{
		return raw_begin();
	}

	inline auto utf8_string_t::raw_begin() const -> char const*
	{
		return data_;
	}

	inline auto utf8_string_t::raw_end() const -> char const*
	{
		return data_ + size_;
	}

	inline auto utf8_string_t::begin() const -> const_iterator
	{
		return const_iterator(raw_begin());
	}

	inline auto utf8_string_t::end() const -> const_iterator
	{
		return const_iterator(raw_end());
	}

	inline auto utf8_string_t::rbegin() const -> const_reverse_iterator
	{
		return const_reverse_iterator(end());
	}

	inline auto utf8_string_t::rend() const -> const_reverse_iterator
	{
		return const_reverse_iterator(begin());
	}

	inline auto utf8_string_t::push_back(char c) -> void
	{
		ATMA_ASSERT(c ^ 0x80);

		imem_quantize_grow(1);

		data_[size_++] = c;
		data_[size_] = '\0';
	}

	inline auto utf8_string_t::push_back(utf8_char_t c) -> void
	{
		imem_quantize_grow(c.size_bytes());

		memcpy(data_ + size_, c.data(), c.size_bytes());
		size_ += c.size_bytes();
		data_[size_] = '\0';
	}

	inline auto utf8_string_t::append(char const* begin, char const* end) -> utf8_string_t&
	{
		if (auto sz = end - begin)
		{
			imem_quantize_grow(sz);

			memcpy(data_ + size_, begin, sz);
			size_ += sz;
			data_[size_] = '\0';
		}

		return *this;
	}

	inline auto utf8_string_t::append(char const* str) -> utf8_string_t&
	{
		append(str, str + strlen(str));
		return *this;
	}

	inline auto utf8_string_t::append(utf8_string_t const& str) -> utf8_string_t&
	{
		append(str.raw_begin(), str.raw_end());
		return *this;
	}

#if 0
	inline auto utf8_string_t::append(utf8_const_span_t const& str) -> utf8_string_t&
	{
		append(str.begin(), str.end());
		return *this;
	}
#endif // 0


	inline auto utf8_string_t::clear() -> void
	{
		size_ = 0;
	}

	inline auto utf8_string_t::imem_quantize(size_t size, bool keep) -> void
	{
		size_t newcap = imem_quantize_capacity(size);

		if (newcap != capacity_)
			imem_realloc(newcap, keep);
	}

	inline auto utf8_string_t::imem_quantize_grow(size_t size) -> void
	{
		// less-than *or equal* so that we always have space for our null-terminator
		if (capacity_ <= size_ + size)
			imem_realloc(imem_quantize_capacity(size_ + size), true);
	}

	inline auto utf8_string_t::imem_quantize_capacity(size_t size) const -> size_t
	{
		size_t result = 8;
		while (size >= 8)
		{
			result *= 2;
			size /= 2;
		}

		return result;
	}

	inline auto utf8_string_t::imem_realloc(size_t capacity, bool keep) -> void
	{
		if (keep && data_)
		{
			auto tmp = data_;
			capacity_ = capacity;
			data_ = new char[capacity_];
			memcpy(data_, tmp, size_);
			data_[size_] = '\0';
			delete [] tmp;
		}
		else
		{
			delete [] data_;
			capacity_ = capacity;
			data_ = new char[capacity_];
		}
	}




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator == (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.size_bytes() == rhs.size_bytes() && ::memcmp(lhs.raw_begin(), rhs.raw_begin(), lhs.size_bytes()) == 0;
	}

	inline auto operator != (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator == (utf8_string_t const& lhs, char const* rhs) -> bool
	{
		return ::strcmp(lhs.raw_begin(), rhs) == 0;
	}

	inline auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return std::lexicographical_compare(lhs.raw_begin(), lhs.raw_end(), rhs.raw_begin(), rhs.raw_end());
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result += rhs;
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, char const* rhs) -> utf8_string_t
	{
		auto result = lhs;
		result += rhs;
		return result;
	}

#if 0
	inline auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		//result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		result.insert(result.end(), rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_const_span_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		return result;
	}
#endif

	inline auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&
	{
		if (!rhs.empty())
			out.write(rhs.raw_begin(), rhs.size_bytes());
		return out;
	}
}


//=====================================================================
// utf8-byte functions
//=====================================================================
namespace atma
{
	// there are a few values that are just not allowed anywhere in an utf8 encoding
	constexpr auto utf8_byte_is_valid(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] != -1;
	}

	constexpr auto utf8_byte_is_leading(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] >= 1;
	}

	constexpr auto utf8_byte_is_run_on(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] == 0;
	}

	constexpr auto utf8_byte_length(byte const c) -> int
	{
		ATMA_ASSERT(utf8_byte_is_leading(c));
		return detail::char_length_table[(uint8)c];
	}
}

//=====================================================================
// utf8-char functions
//=====================================================================
namespace atma
{
	// c in codepoint [0, 128), a.k.a: ascii
	constexpr auto utf8_char_is_ascii(char const c) -> bool
	{
		return detail::char_length_table[c] == 1;
	}

	constexpr auto utf8_char_is_newline(char const* leading) -> bool
	{
		ATMA_ASSERT(leading);

		return
			*leading == 0x0a ||
			*leading == 0x0b ||
			*leading == 0x0c ||
			*leading == 0x0d ||
			*leading == 0x85;
	}

	// return how many bytes we need to advance, assuming we're at a leading byte
	constexpr auto utf8_char_size_bytes(char const* leading) -> int
	{
		ATMA_ASSERT(leading);
		ATMA_ENSURE(utf8_byte_is_leading((byte)*leading));

		return detail::char_length_table[*leading];
	}

	constexpr auto utf8_char_advance(char const*& leading) -> void
	{
		ATMA_ASSERT(leading);

		leading += utf8_char_size_bytes(leading);
	}

	constexpr auto utf8_char_equality(char const* lhs, char const* rhs) -> bool
	{
		ATMA_ASSERT(lhs);
		ATMA_ASSERT(rhs);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*lhs));
		ATMA_ASSERT(utf8_byte_is_leading((byte)*rhs));

		if (*lhs != *rhs)
			return false;

		return memcmp(lhs + 1, rhs + 1, utf8_char_size_bytes(lhs) - 1) == 0;
	}

#if 0
	inline auto utf8_char_codepoint(utf8_char_t x) -> codepoint
	{
		ATMA_ASSERT(x);
		ATMA_ASSERT(utf8_byte_is_leading(*x));

		codepoint r = 0;

		if ((x[0] & 0x80) == 0)
			r = *x;
		else if ((x[0] & 0xe0) == 0xc0)
			r = ((x[0] & 0x1f) << 6) | (x[1] & 0x3f);
		else if ((x[0] & 0xf0) == 0xe0)
			r = ((x[0] & 0x0f) << 12) | ((x[1] & 0x3f) << 6) | (x[2] & 0x3f);
		else if ((x[0] & 0xf8) == 0xf0)
			r = ((x[0] & 0x07) << 18) | ((x[1] & 0x3f) << 12) | ((x[2] & 0x3f) << 6) | (x[3] & 0x3f);

		return r;
	}
#endif
}



//=====================================================================
//
//  @charseq
//  -----------
//  operations on a sequence of utf8-chars. "char const*" is
//  indistinguishable when used as both a single char or many
//
namespace atma
{
	template <typename FN>
	inline auto utf8_charseq_for_each(char const* seq, FN&& fn) -> void
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			fn(i);
	}

	template <typename T>
	inline auto utf8_charseq_any_of(char const* seq, T&& pred) -> bool
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			if (pred(i))
				return true;

		return false;
	}

	inline auto utf8_charseq_find(char const* seq, utf8_char_t x) -> char const*
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			if (strncmp(x.data(), i, x.size_bytes()) == 0)
				return i;

		return nullptr;
	}

	inline auto utf8_charseq_idx_to_byte_idx(char const* seq, size_t sz, size_t char_idx) -> size_t
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT((sz == 0 && char_idx == 0) || utf8_byte_is_leading((byte)*seq));

		size_t r = 0;
		char const* i = seq;
		for (auto ie = seq + sz; i != ie && r != char_idx; i += utf8_char_size_bytes(i))
			++r;

		return (i - seq);
	}
}



namespace atma
{

	//=====================================================================
	// functions
	//=====================================================================
#if 0
	inline auto strcpy(utf8_string_t& dest, utf8_string_t const& src) -> void
	{
		ATMA_ASSERT(dest.size_bytes() >= src.size_bytes());

		auto i = dest.begin();
		for (auto& x : src)
			*i++ = x;
	}
#endif


	//
	// find_if
	//
	template <utf8_charseq_range_concept<char const> Range, typename Predicate>
	inline auto find_if(Range const& range, Predicate&& pred)
	{
		return find_if(std::begin(range), std::end(range), std::forward<Predicate>(pred));
	}

	template <utf8_charseq_iterator_concept Iterator, utf8_char_fn_concept Predicate>
	inline auto find_if(Iterator begin, Iterator end, Predicate&& pred) -> Iterator
	{
		auto i = begin;
		for (; i != end; ++i)
			if (pred(utf8_char_t(std::to_address(i))))
				break;

		return i;
	}

	template <utf8_iterator_concept Iterator, utf8_char_fn_concept Predicate>
	inline auto find_if(Iterator begin, Iterator end, Predicate&& pred) -> Iterator
	{
		auto i = begin;
		for (; i != end; ++i)
			if (pred(*i))
				break;

		return i;
	}

	

	//
	// find_first_of
	//
	template <utf8_fwding_iterator_concept Iterator>
	inline auto find_first_of(Iterator begin, Iterator end, char const* delims) -> Iterator
	{
		ATMA_ASSERT(delims);

		return find_if(begin, end, [delims](utf8_char_t lhs) {
			return utf8_charseq_any_of(delims, [lhs](char const* delim) {
				return lhs == delim; }); });
	}

	template <utf8_charseq_range_concept Range>
	inline auto find_first_of(Range const& range, char const* delims)
	{
		return find_first_of(std::begin(range), std::end(range), delims);
	}

	template <utf8_range_concept Range>
	inline auto find_first_of(Range const& range, char const* delims)
	{
		return find_first_of(std::begin(range), std::end(range), delims);
	}

	template <typename Range>
	inline auto find_first_of(Range const& range, char x)
	{
		char delims[] = {x, '\0'};
		return find_first_of(range, delims);
	}


	//
	// utf8_appender_t
	//
	struct utf8_appender_t
	{
		utf8_appender_t(utf8_string_t& dest)
			: str_(dest)
		{}

		template <typename T>
		auto operator ()(T&& x) const -> utf8_appender_t const&
		{
			str_.append(std::forward<T>(x));
			return *this;
		}

	private:
		utf8_string_t& str_;
	};


}

