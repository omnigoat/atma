#pragma once

#include <atma/types.hpp>
#include <atma/assert.hpp>


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
	inline auto utf8_byte_is_leading(char const c) -> bool
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



	struct utf8_char_t
	{
		utf8_char_t(char const* c)
			: c(c)
		{}

		auto operator [](int i) -> char { return c[i]; }

		auto bytecount() const -> size_t { return utf8_char_bytecount(c); }
		auto data() const -> char const* { return c; }

		operator char const*() const { return c; }

		auto operator = (utf8_char_t const& rhs) -> utf8_char_t&
		{
			c = rhs.c;
			return *this;
		}

	private:
		char const* c;
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

	template <typename T>
	inline auto utf8_charseq_find(char const* seq, utf8_char_t x) -> char const*
	{
		ATMA_ASSERT(seq);
		ATMA_ASSERT(utf8_byte_is_leading(*seq));

		for (auto i = seq; *i; utf8_char_advance(i))
			if (x == i)
				return i;

		return nullptr;
	}

}
