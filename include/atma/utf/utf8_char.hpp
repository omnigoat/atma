#pragma once
//=====================================================================
#include <atma/types.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
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
	//=====================================================================



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

			4, 4, 4, 4, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		};

		char const null_byte = '\0';
	}

	// there are a few values that are just not allowed anywhere in an utf8 encoding.
	inline auto utf8_byte_is_valid(char const c) -> bool
	{
		return detail::char_length_table[c] != -1;
	}

	// c is a valid utf8 byte and the leading byte of a multi-byte sequence
	inline auto utf8_byte_is_leading(char const c) -> bool {
		return detail::char_length_table[c] > 1;
	}

	// c in [0, 128)
	inline auto utf8_char_is_ascii(char const c) -> bool {
		return detail::char_length_table[c] == 1;
	}

	// return how many bytes we need to advance, assuming we're at a leading byte
	inline auto utf8_char_bytecount(char const* leading) -> bool {
		ATMA_ASSERT(leading);
		ATMA_ENSURE(utf8_char_is_leading(*leading));
		return detail::char_length_table[*leading];
	}


	//=====================================================================
	// utf8_char_t
	// -------------
	//   a sequence of bytes that make up a utf8 character encoding.
	//
	//   there is no "null" state for this byte, the default state is the
	//   codepoint zero.
	//=====================================================================
	struct utf8_char_t
	{
		utf8_char_t()
			: begin(&detail::null_byte), end(&detail::null_byte + 1)
		{}

		utf8_char_t(char const* begin, char const* end)
			: begin(begin), end(end)
		{
			ATMA_ASSERT(begin);
			ATMA_ASSERT(end);
		}

		utf8_char_t(utf8_char_t const& rhs)
			: begin(rhs.begin), end(rhs.end)
		{
			ATMA_ASSERT(begin);
			ATMA_ASSERT(end);
		}


		auto operator = (utf8_char_t const& rhs) -> utf8_char_t&
		{
			begin = rhs.begin;
			end = rhs.end;
			return *this;
		}

		char const* begin;
		char const* end;
	};

	inline auto operator == (utf8_char_t const& lhs, utf8_char_t const& rhs) -> bool {
		return (lhs.end - lhs.begin) == (rhs.end - rhs.begin)
			&& std::equal(lhs.begin, lhs.end, rhs.begin)
			;
	}

	inline auto operator == (utf8_char_t const& lhs, char x) -> bool {
		// it only makes sense to compare a character against a character, and
		// not a character against a leading-byte in a char-seq.
		ATMA_ASSERT(utf8_char_is_ascii(x));

		return *lhs.begin == x;
	}

//=====================================================================
} // namespace atma
//=====================================================================
