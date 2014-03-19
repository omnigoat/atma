#pragma once
//=====================================================================
#include <atma/types.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	namespace detail
	{
		int const char_seq_length_table[] =
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
	}

	// there are a few values that are just not allowed anywhere in an utf8 encoding.
	inline auto is_valid_utf8_char(char const c) -> bool
	{
		return detail::char_seq_length_table[c] != -1;
	}

	// c in [0, 128)
	inline auto utf8_char_is_ascii(char const c) -> bool {
		return detail::char_seq_length_table[c] == 1;
	}

	// c is a valid utf8 char and the leading byte of a multi-byte sequence
	inline auto utf8_char_is_leading(char const c) -> bool {
		return detail::char_seq_length_table[c] > 1;
	}

	// return how many bytes we need to advance, assuming we're at a leading byte
	inline auto utf8_charseq_length(char const* leading) -> bool {
		ATMA_ASSERT(leading);
		ATMA_ENSURE(utf8_char_is_leading(*leading));
		return detail::char_seq_length_table[*leading];
	}

	inline auto utf8_charseq_advance(char const* begin) -> char const* {
		ATMA_ASSERT(begin);
		return begin + utf8_charseq_length(begin);
	}


	template <typename OT, typename IT>
	inline OT utf16_from_utf8(OT dest, IT begin, IT end)
	{
		static_assert(std::is_convertible<decltype(*begin), char const>::value, "value_type of input iterators must be char");

		for (auto i = begin; i != end; ++i)
		{
			// *must* be char
			char const& x = *i;


			// top 4 bits set, 21 bits (surrogates!)
			if ((x & 0xf8) == 0xf0)
			{
				char32_t surrogate =
					static_cast<char32_t>((x & 0x7) << 18) |
					static_cast<char32_t>((*++i & 0x3f) << 12) |
					static_cast<char32_t>((*++i & 0x3f) << 6) |
					static_cast<char32_t>(*++i & 0x3f)
					;

				surrogate -= 0x10000;
				*dest++ = static_cast<char16_t>(surrogate >> 10) + 0xd800;
				*dest++ = static_cast<char16_t>(surrogate & 0xffa00) + 0xdc00;
			}
			// top 3 bits set, 16 bits
			else if ((x & 0xf0) == 0xe0) {
				*dest++ =
					(static_cast<char16_t>(x & 0x0f) << 12) |
					(static_cast<char16_t>(*++i & 0x3f) << 6) |
					static_cast<char16_t>(*++i & 0x3f)
					;
			}
			// top 2 bits set, 12 bits
			else if ((x & 0xe0) == 0xc0) {
				*dest++ =
					(static_cast<char16_t>(x & 0x3f)  << 6) |
					static_cast<char16_t>(*++i & 0x3f)
					;
			}
			// one byte! 7 bits
			else {
				*dest++ = static_cast<char16_t>(x);
			}
		}

		return dest;
	}



//=====================================================================
} // namespace atma
//=====================================================================
