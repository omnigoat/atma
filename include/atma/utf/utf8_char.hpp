#pragma once

#include <atma/types.hpp>
#include <atma/assert.hpp>

#include <atma/ranges/core.hpp>

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
namespace atma
{
	using codepoint = uint32;

	namespace detail
	{
		int const char_length_table[] =
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

		char const zero_byte = '\0';
	}


	// there are a few values that are just not allowed anywhere in an utf8 encoding.
	inline auto utf8_byte_is_valid(char const c) -> bool
	{
		return detail::char_length_table[c] != -1;
	}

	// c is a valid utf8 byte and the leading byte of a sequence
	constexpr inline auto utf8_byte_is_leading(char const c) -> bool
	{
		return detail::char_length_table[c] >= 1;
	}

	inline auto utf8_byte_is_run_on(char const c) -> bool
	{
		return detail::char_length_table[c] == 0;
	}

	// c in codepoint [0, 128), a.k.a: ascii
	inline auto utf8_char_is_ascii(char const c) -> bool
	{
		return detail::char_length_table[c] == 1;
	}

	inline auto utf8_char_is_newline(char const* leading) -> bool
	{
		return
			*leading == 0x0a ||
			*leading == 0x0b ||
			*leading == 0x0c ||
			*leading == 0x0d ||
			*leading == 0x85;
	}

	// return how many bytes we need to advance, assuming we're at a leading byte
	inline auto utf8_char_bytecount(char const* leading) -> int
	{
		ATMA_ASSERT(leading);
		ATMA_ENSURE(utf8_byte_is_leading(*leading));
		return detail::char_length_table[*leading];
	}

	inline auto utf8_char_advance(char const*& leading) -> void
	{
		leading += utf8_char_bytecount(leading);
	}

	inline auto utf8_char_equality(char const* lhs, char const* rhs) -> bool
	{
		ATMA_ASSERT(lhs);
		ATMA_ASSERT(rhs);
		ATMA_ASSERT(utf8_byte_is_leading(*lhs));
		ATMA_ASSERT(utf8_byte_is_leading(*rhs));

		if (*lhs != *rhs)
			return false;

		return memcmp(lhs + 1, rhs + 1, utf8_char_bytecount(lhs) - 1) == 0;
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


//
// utf8_char_t
// -------------
//  does not own the backing byte sequence
//
namespace atma
{
	struct utf8_char_t
	{
		using size_type = size_t;

		constexpr utf8_char_t() = default;
		constexpr utf8_char_t(utf8_char_t const&) = default;

		constexpr utf8_char_t(char const* c)
			: c(c)
		{
			ATMA_ASSERT(c);
			ATMA_ASSERT(utf8_byte_is_leading(*c));
		}

		auto operator [](size_type idx) -> char
		{
			ATMA_ASSERT(idx < bytecount());
			return c[idx];
		}

		auto bytecount() const -> size_t { return utf8_char_bytecount(c); }
		auto data() const -> char const* { return c; }

		operator char const*() const { return c; }

		auto operator = (utf8_char_t const&) -> utf8_char_t& = default;

	private:
		char const* c = "";
	};

	static_assert(sizeof(utf8_char_t) == sizeof(char const*), "what happened?");

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
		return lhs[0] == rhs;
	}

	inline auto operator != (utf8_char_t lhs, char rhs) -> bool
	{
		return lhs[0] != rhs;
	}
}




//
// utf8_iterator_t
// -----------------
//  given a sequence of chars, iterates character-by-character. this
//  means that the amount of bytes traversed is variable, as utf8 is
//  want to be
//
namespace atma
{
#if 0
	struct utf8_iterator_t
	{
		utf8_iterator_t() = default;
		utf8_iterator_t(utf8_iterator_t const&) = default;
		utf8_iterator_t(char*);

		// access
		auto operator *() const -> char;

		// travel
		auto operator ++() -> utf8_iterator_t&;
		auto operator --() -> utf8_iterator_t&;
		auto operator ++(int) -> utf8_iterator_t;
		auto operator --(int) -> utf8_iterator_t;

	private:
		char* here_ = nullptr;
	};

	inline auto utf8_iterator_t::operator++ () -> utf8_iterator_t&
	{
		here_ = utf8_next_char(here_);
		return *this;
	}

	inline auto utf8_iterator_t::operator *() -> char
	{
		return *here_;
	}
#endif
}



//=====================================================================
// charseq
// ---------
//  operations on a sequence of utf8-chars. "char const*" is
//  indistinguishable when used as both a single char or many
//=====================================================================
namespace atma
{
	template <typename FN>
	inline auto utf8_charseq_for_each(char const* seq, FN&& fn) -> void
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading(*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			fn(i);
	}

	template <typename T>
	inline auto utf8_charseq_any_of(char const* seq, T&& pred) -> bool
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading(*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			if (pred(i))
				return true;

		return false;
	}

	inline auto utf8_charseq_find(char const* seq, utf8_char_t x) -> char const*
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading(*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			if (x == i)
				return i;

		return nullptr;
	}

	inline auto utf8_charseq_idx_to_byte_idx(char const* seq, size_t sz, size_t char_idx) -> size_t
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading(*seq));

		size_t r = 0;
		char const* i = seq;
		for (auto ie = seq + sz; i != ie && r != char_idx; i += utf8_char_bytecount(i))
			++r;

		return (i - seq);
	}
}


//
namespace atma
{
	//struct utf8_span_t
	//{
	//	constexpr utf8_span_t() = default;
	//	utf8_span_t(utf8_span_t const&) = default;
	//	constexpr ~utf8_span_t() = default;
	//
	//	constexpr utf8_span_t(char const* data, size_t size);
	//
	//	// mutable access
	//	constexpr auto begin() { return data_; }
	//	constexpr auto end() { return data_ + size_; }
	//	constexpr auto data() { return data_; }
	//
	//	// immutable access
	//	auto begin() const { return data_; }
	//	auto end() const { return data_ + size_; }
	//	auto data() const { return data_; }
	//
	//	auto size() const { return size_; }
	//	auto size_bytes() const { return size_; }
	//	auto empty() const { return size_ == 0; }
	//
	//	//auto 
	//
	//private:
	//	char const* data_ = "";
	//	size_t size_ = 0;
	//};

	using utf8_span_t = span_t<char>;
	using utf8_const_span_t = span_t<char const>;
}

