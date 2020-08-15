#pragma once

#include <atma/utf/utf8_string_range_header.hpp>
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
// functions
//=====================================================================
namespace atma
{
	// there are a few values that are just not allowed anywhere in an utf8 encoding.
	constexpr auto utf8_byte_is_valid(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] != -1;
	}

	// c is a valid utf8 byte and the leading byte of a sequence
	constexpr auto utf8_byte_is_leading(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] >= 1;
	}

	constexpr auto utf8_byte_is_run_on(byte const c) -> bool
	{
		return detail::char_length_table[(uint8)c] == 0;
	}

	// c in codepoint [0, 128), a.k.a: ascii
	constexpr auto utf8_char_is_ascii(char const c) -> bool
	{
		return detail::char_length_table[c] == 1;
	}

	constexpr auto utf8_leading_byte_length(byte const c) -> int
	{
		return detail::char_length_table[(uint8)c];
	}

	constexpr auto utf8_char_is_newline(char const* leading) -> bool
	{
		return
			*leading == 0x0a ||
			*leading == 0x0b ||
			*leading == 0x0c ||
			*leading == 0x0d ||
			*leading == 0x85;
	}


	// return how many bytes we need to advance, assuming we're at a leading byte
	inline auto utf8_char_size_bytes(char const* leading) -> int
	{
		ATMA_ASSERT(leading);
		ATMA_ENSURE(utf8_byte_is_leading((byte)*leading));
		return detail::char_length_table[*leading];
	}

	inline auto utf8_char_advance(char const*& leading) -> void
	{
		leading += utf8_char_size_bytes(leading);
	}

	inline auto utf8_char_equality(char const* lhs, char const* rhs) -> bool
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
//  CONCEPTS
//
namespace atma::detail
{
	// a standards-compliant contiguous range that contains
	// elements of type BackingType
	template <typename R, typename BackingType>
	concept utf8_range_concept =
		std::ranges::contiguous_range<R> &&
		std::is_convertible_v<std::ranges::range_value_t<R>, BackingType>;

	// a standards-compliant contiguous iterator that
	// dereferences to a value-type of BackingType
	template <typename It, typename BackingType>
	concept utf8_iterator_concept =
		std::contiguous_iterator<It> &&
		std::is_convertible_v<std::iter_value_t<It>*, BackingType* const>;
}


//=====================================================================
//
//  @ utf8_char_t
//  ---------------
//  
//
namespace atma
{
	struct utf8_char_t
	{
		constexpr utf8_char_t() = default;
		constexpr utf8_char_t(utf8_char_t const&) = default;

		constexpr utf8_char_t(char const*);
		//constexpr explicit utf8_char_t(char32_t);
		//constexpr utf8_char_t(char16_t, char16_t = 0);

		constexpr auto operator [](size_t idx) const -> byte
		{
			ATMA_ASSERT(idx < size_bytes());
			return bytes_[idx];
		}

		auto data() const -> char const* { return reinterpret_cast<char const*>(bytes_); }
		constexpr auto size_bytes() const -> size_t { return utf8_leading_byte_length(bytes_[0]); }

		explicit operator char const* () const { return data(); }

		auto operator = (utf8_char_t const&) -> utf8_char_t& = default;

	private:
		byte bytes_[4] = {byte(), byte(), byte(), byte()};
	};

}


//=====================================================================
//
//  @ charseq
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
		ATMA_ASSERT(utf8_byte_is_leading((byte)*seq));

		size_t r = 0;
		char const* i = seq;
		for (auto ie = seq + sz; i != ie && r != char_idx; i += utf8_char_size_bytes(i))
			++r;

		return (i - seq);
	}
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
//  @ utf8_span_t
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
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = T const*;
		using reference = T&;
		using const_reference = T const&;
		using iterator = element_type*;
		using reverse_iterator = std::reverse_iterator<iterator>;

		constexpr basic_utf8_span_t() = default;
		constexpr basic_utf8_span_t(basic_utf8_span_t const&) = default;

		// construct from a range of compatible values
		template <utf8_range_concept<T> Range>
		constexpr basic_utf8_span_t(Range const& range)
			: data_(std::data(range))
			, size_(std::size(range))
		{}

		constexpr basic_utf8_span_t(element_type* data, size_t size)
			: data_(data)
			, size_(size)
		{}

		template <utf8_iterator_concept<T> Iterator>
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
		element_type* data_ = nullptr;
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
//  @ utf8_iterator_t
//  -------------------
//  given a sequence of chars, iterates character-by-character. this
//  means that the amount of bytes traversed is variable, as utf8 is
//  want to be
//
namespace atma::detail
{
	template <typename T>
	struct basic_utf8_iterator_t
	{
		using character_backing_type = T;
		using element_type = transfer_const_t<character_backing_type, utf8_char_t>; 
		using value_type = std::remove_cv_t<element_type>;

		basic_utf8_iterator_t() = default;
		basic_utf8_iterator_t(basic_utf8_iterator_t const&) = default;
		basic_utf8_iterator_t(character_backing_type*);

		// access
		auto operator *() const { return value_type{here_}; }

		// travel
		auto operator ++() -> basic_utf8_iterator_t&;
		auto operator ++(int) -> basic_utf8_iterator_t;
		auto operator --() -> basic_utf8_iterator_t&;
		auto operator --(int) -> basic_utf8_iterator_t;

		auto char_data() const { return here_; }

	private:
		character_backing_type* here_ = nullptr;
	};
}

namespace atma
{
	using utf8_iterator_t = detail::basic_utf8_iterator_t<char>;
	using utf8_const_iterator_t = detail::basic_utf8_iterator_t<char const>;
}


//=====================================================================
//
//  @ basic_utf8_range_t
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
		using iterator = detail::basic_utf8_iterator_t<backing_type>;

		basic_utf8_range_t();
		basic_utf8_range_t(basic_utf8_range_t const&);

		// construct from a range of compatible values
		template <utf8_range_concept<BackingType> Range>
		explicit basic_utf8_range_t(Range const&);

		// conversion from iterator-pairs
		template <utf8_iterator_concept<BackingType> IteratorType>
		basic_utf8_range_t(IteratorType, IteratorType);

		auto raw_size() const -> size_t;
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
// utf8_string
//
//=====================================================================
namespace atma
{
	struct utf8_string_t
	{
		struct iterator_t;

		using value_t                = char;
		using iterator               = iterator_t;
		using const_iterator         = iterator_t;
		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		utf8_string_t();
		utf8_string_t(char const* str, size_t size);
		utf8_string_t(char const* str_begin, char const* str_end);
		utf8_string_t(char const* str);
		utf8_string_t(const_iterator const&, const_iterator const&);

		//template <typename T>
		//requires 
		//utf8_string_t(utf8_const_span_t const&);

		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		~utf8_string_t();

		auto operator = (utf8_string_t const&) -> utf8_string_t&;
		auto operator = (utf8_string_t&&) -> utf8_string_t&;
		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&;
		auto operator += (char const*) -> utf8_string_t&;


		auto empty() const -> bool;
		auto c_str() const -> char const*;
		auto raw_size() const -> size_t;

		auto begin() const -> const_iterator;
		auto end() const -> const_iterator;
		auto rbegin() const -> const_reverse_iterator;
		auto rend() const -> const_reverse_iterator;

		auto raw_begin() const -> char const*;
		auto raw_end() const -> char const*;

		auto raw_iter_of(const_iterator const&) const -> char const*;

		auto push_back(char) -> void;
		auto push_back(utf8_char_t) -> void;

		auto append(char const*, char const*) -> void;
		auto append(char const*) -> void;
		auto append(utf8_string_t const&) -> void;

		auto append(utf8_const_span_t const&) -> void;

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
//
// utf8_string::iterator
//
//=====================================================================
namespace atma
{
	struct utf8_string_t::iterator_t
	{
	private:
		friend struct utf8_string_t;

		using owner_t = utf8_string_t;

	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = ptrdiff_t;
		using value_type        = utf8_char_t;
		using element_type      = utf8_char_t;
		using pointer           = utf8_char_t*;
		using reference         = utf8_char_t const&;

		iterator_t(iterator_t const&);

		auto operator = (iterator_t const&) -> iterator_t&;
		auto operator ++ () -> iterator_t&;
		auto operator ++ (int) -> iterator_t;
		auto operator -- () -> iterator_t&;
		auto operator -- (int) -> iterator_t;

		auto operator * () const -> reference;
		auto operator -> () const -> pointer;

	private:
		iterator_t(owner_t const*, char const*);

	private:
		owner_t const* owner_;
		char const* ptr_;
		mutable utf8_char_t ch_;

		friend auto operator == (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;
	};


	inline auto operator == (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;
	inline auto operator != (utf8_string_t::iterator_t const&, utf8_string_t::iterator_t const&) -> bool;
}






//
// utf8_char_t implementation
//
namespace atma
{
	constexpr inline utf8_char_t::utf8_char_t(char const* c)
	{
		ATMA_ASSERT(c != nullptr);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*c));

		auto sz = detail::char_length_table[*c];

		// take advantage of short circuiting here, as no byte
		// in a valid utf8 sequence should equate to zero
		(sz-- && (bool)(bytes_[0] = (byte)*c++)) &&
		(sz-- && (bool)(bytes_[1] = (byte)*c++)) &&
		(sz-- && (bool)(bytes_[2] = (byte)*c++)) &&
		(sz-- && (bool)(bytes_[3] = (byte)*c++));
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
		return lhs[0] == byte{rhs};
	}

	inline auto operator != (utf8_char_t lhs, char rhs) -> bool
	{
		ATMA_ASSERT(utf8_char_is_ascii(rhs));
		return lhs[0] != byte{rhs};
	}
}


//
// basic_utf8_iterator_t implementation
//
namespace atma::detail
{
	template <typename T>
	inline basic_utf8_iterator_t<T>::basic_utf8_iterator_t(character_backing_type* here)
		: here_(here)
	{}

	template <typename T>
	inline auto basic_utf8_iterator_t<T>::operator++ () -> basic_utf8_iterator_t&
	{
		here_ += utf8_char_size_bytes(here_);
		return *this;
	}

	template <typename T>
	inline auto basic_utf8_iterator_t<T>::operator++ (int) -> basic_utf8_iterator_t
	{
		auto r = *this;
		++r;
		return r;
	}

	template <typename T>
	inline auto basic_utf8_iterator_t<T>::operator-- () -> basic_utf8_iterator_t&
	{
		while (!utf8_byte_is_leading(*--here_));
		return *this;
	}

	template <typename T>
	inline auto basic_utf8_iterator_t<T>::operator-- (int) -> basic_utf8_iterator_t
	{
		auto r = *this;
		--r;
		return r;
	}

	template <typename T>
	inline auto operator == (basic_utf8_iterator_t<T> const& lhs, basic_utf8_iterator_t<T> const& rhs)
	{
		return lhs.char_data() == rhs.char_data();
	}

	template <typename T>
	inline auto operator != (basic_utf8_iterator_t<T> const& lhs, basic_utf8_iterator_t<T> const& rhs)
	{
		return !operator == (lhs, rhs);
	}
}

//=====================================================================
// !utf8_span_t implementation
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
// @ basic_utf8_range_t implementation
//=====================================================================
namespace atma::detail
{
	template <typename BT>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t()
		: begin_(), end_()
	{}

	template <typename BT>
	template <utf8_range_concept<BT> Range>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(Range const& range)
	{
	}

	template <typename BT>
	template <utf8_iterator_concept<BT> IteratorType>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(IteratorType begin, IteratorType end)
		: begin_(std::to_address(begin)), end_(std::to_address(end))
	{}

	template <typename BT>
	inline basic_utf8_range_t<BT>::basic_utf8_range_t(basic_utf8_range_t const& rhs)
		: begin_(rhs.begin_), end_(rhs.end_)
	{}

	template <typename BT>
	inline auto basic_utf8_range_t<BT>::raw_size() const -> size_t
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
	template <typename BT>
	inline auto operator == (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.begin(), rhs.begin(), lhs.raw_size()) == 0;
	}

	template <typename BT>
	inline auto operator != (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	template <typename BT>
	inline auto operator == (basic_utf8_range_t<BT> const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.begin(), rhs.raw_begin(), lhs.raw_size()) == 0;
	}

	template <typename BT>
	inline auto operator == (utf8_string_t const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.raw_begin(), rhs.begin(), lhs.raw_size()) == 0;
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
		return strncmp(lhs.begin(), rhs, lhs.raw_size()) == 0;
	}

	template <typename BT>
	inline auto operator < (basic_utf8_range_t<BT> const& lhs, basic_utf8_range_t<BT> const& rhs) -> bool
	{
		auto cmp = ::strncmp(
			lhs.begin(), rhs.begin(),
			std::min(lhs.raw_size(), rhs.raw_size()));

		return
			cmp < 0 || (!(0 < cmp) && (
				lhs.raw_size() < rhs.raw_size()));
	}

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
//
// utf8_string implementation
//
//=====================================================================
namespace atma
{
	inline utf8_string_t::utf8_string_t()
		: capacity_(8)
		, size_()
		, data_(new char[capacity_])
	{
		memset(data_, 0, capacity_);
	}

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
	{
	}

	inline utf8_string_t::utf8_string_t(char const* str)
		: utf8_string_t(str, strlen(str))
	{
	}

	inline utf8_string_t::utf8_string_t(const_iterator const& begin, const_iterator const& end)
		: utf8_string_t(begin->data(), end->data() - begin->data())
	{
	}

	inline utf8_string_t::utf8_string_t(utf8_string_t const& str)
		: utf8_string_t(str.raw_begin(), str.raw_size())
	{
	}

#if 0
	inline utf8_string_t::utf8_string_t(utf8_const_span_t const& range)
		: utf8_string_t{range.begin(), range.raw_size()}
	{
	}
#endif // 0


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
		delete data_;
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

	inline auto utf8_string_t::raw_size() const -> size_t
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
		return const_iterator(this, raw_begin());
	}

	inline auto utf8_string_t::end() const -> const_iterator
	{
		return const_iterator(this, raw_end());
	}

	inline auto utf8_string_t::rbegin() const -> const_reverse_iterator
	{
		return const_reverse_iterator(end());
	}

	inline auto utf8_string_t::rend() const -> const_reverse_iterator
	{
		return const_reverse_iterator(begin());
	}

	inline auto utf8_string_t::raw_iter_of(const_iterator const& iter) const -> char const*
	{
		return iter.ptr_;
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
#if 0
		auto sz = std::distance(c.begin, c.end);
		imem_quantize_grow(sz);

		memcpy(data_ + size_, c.begin, sz);
		size_ += sz;
		data_[size_] = '\0';
#endif
	}

	inline auto utf8_string_t::append(char const* begin, char const* end) -> void
	{
		auto sz = end - begin;
		if (sz == 0)
			return;
		imem_quantize_grow(sz);

		memcpy(data_ + size_, begin, sz);
		size_ += sz;
		data_[size_] = '\0';
	}

	inline auto utf8_string_t::append(char const* str) -> void
	{
		append(str, str + strlen(str));
	}

	inline auto utf8_string_t::append(utf8_string_t const& str) -> void
	{
		append(str.raw_begin(), str.raw_end());
	}

#if 0
	inline auto utf8_string_t::append(utf8_const_span_t const& str) -> void
	{
		append(str.begin(), str.end());
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
			delete tmp;
		}
		else
		{
			delete data_;
			capacity_ = capacity;
			data_ = new char[capacity_];
		}
	}




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator == (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && ::memcmp(lhs.raw_begin(), rhs.raw_begin(), lhs.raw_size()) == 0;
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
			out.write(rhs.raw_begin(), rhs.raw_size());
		return out;
	}




	//=====================================================================
	// iterator
	//=====================================================================
	inline utf8_string_t::iterator_t::iterator_t(owner_t const* owner, char const* ptr)
		: owner_(owner)
		, ptr_(ptr)
		, ch_{ptr}
	{
		ATMA_ASSERT(ptr);
		ATMA_ASSERT(utf8_byte_is_leading((byte)*ptr));
	}

	inline utf8_string_t::iterator_t::iterator_t(iterator_t const& rhs)
		: owner_(rhs.owner_)
		, ptr_(rhs.ptr_)
		, ch_{rhs.ptr_}
	{}

	inline auto utf8_string_t::iterator_t::operator = (iterator_t const& rhs) -> iterator_t&
	{
		owner_ = rhs.owner_;
		ptr_ = rhs.ptr_;
		return *this;
	}

	inline auto utf8_string_t::iterator_t::operator ++ () -> iterator_t&
	{
		utf8_char_advance(ptr_);
		return *this;
	}

	inline auto utf8_string_t::iterator_t::operator ++ (int) -> iterator_t
	{
		auto t = *this;
		++*this;
		return t;
	}

	inline auto utf8_string_t::iterator_t::operator -- () -> iterator_t&
	{
		if (ptr_ == owner_->data_)
			return *this;

		while (--ptr_ != owner_->data_ && utf8_byte_is_run_on((byte)*ptr_))
			;

		return *this;
	}

	inline auto utf8_string_t::iterator_t::operator -- (int) -> iterator_t
	{
		auto t = *this;
		--*this;
		return t;
	}

	inline auto utf8_string_t::iterator_t::operator * () const -> reference
	{
		ch_ = utf8_char_t{ptr_};
		return ch_;
	}

	inline auto utf8_string_t::iterator_t::operator -> () const -> pointer
	{
		ch_ = utf8_char_t{ptr_};
		return &ch_;
	}

	inline auto operator == (utf8_string_t::iterator_t const& lhs, utf8_string_t::iterator_t const& rhs) -> bool
	{
		return lhs.ptr_ == rhs.ptr_;
	}

	inline auto operator != (utf8_string_t::iterator_t const& lhs, utf8_string_t::iterator_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}



	//=====================================================================
	// functions
	//=====================================================================
#if 0
	inline auto strcpy(utf8_string_t& dest, utf8_string_t const& src) -> void
	{
		ATMA_ASSERT(dest.raw_size() >= src.raw_size());

		auto i = dest.begin();
		for (auto& x : src)
			*i++ = x;
	}
#endif


	//
	// find_if
	//
	template <typename T>
	inline auto find_if(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end, T&& pred) -> utf8_string_t::const_iterator
	{
		auto i = begin;
		for (; i != end; ++i)
			if (pred(*i))
				break;

		return i;
	}

	template <typename T>
	inline auto find_if(utf8_string_t::const_reverse_iterator const& begin, utf8_string_t::const_reverse_iterator const& end, T&& pred) -> utf8_string_t::const_reverse_iterator
	{
		auto i = begin;
		for (; i != end; ++i)
			if (pred(*i))
				break;

		return i;
	}

	template <typename T>
	inline auto find_if(utf8_string_t const& string, T&& pred) -> utf8_string_t::const_iterator
	{
		return find_if(string.begin(), string.end(), std::forward<T>(pred));
	}

	//
	// find_first_of
	//
	inline auto find_first_of(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end, char const* delims) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(delims);

		return find_if(begin, end, [&](utf8_char_t const& lhs) {
			return utf8_charseq_any_of(delims, [&lhs](utf8_char_t const& rhs) {
				return lhs == rhs; }); });
	}

	inline auto find_first_of(utf8_string_t::const_reverse_iterator const& begin, utf8_string_t::const_reverse_iterator const& end, char const* delims) -> utf8_string_t::const_reverse_iterator
	{
		ATMA_ASSERT(delims);

		return find_if(begin, end, [&](utf8_char_t lhs) {
			return utf8_charseq_any_of(delims, [lhs](utf8_char_t const& rhs) {
				return lhs == rhs; }); });
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& whhere, char const* delims) -> utf8_string_t::const_iterator
	{
		return find_first_of(whhere, str.end(), delims);
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& i, char x) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(utf8_char_is_ascii(x));

		char delims[2] = {x, '\0'};

		return find_first_of(i, str.end(), delims);
	}

	inline auto find_first_of(utf8_string_t const& str, char x) -> utf8_string_t::const_iterator
	{
		return find_first_of(str, str.begin(), x);
	}

	inline auto find_first_of(utf8_string_t const& str, char const* delims) -> utf8_string_t::const_iterator
	{
		return find_first_of(str.begin(), str.end(), delims);
	}

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

