#ifndef ATMA_UTF_ALGORITHM
#define ATMA_UTF_ALGORITHM
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	inline bool is_utf8_leading_byte(char const c) {
		return (c & 0xc0) != 0xa0;
	}

	inline auto is_ascii(char const c) -> bool {
		return (c & 0x80) == 0;
	}

	inline char const* utf8_next_char(char const* begin)
	{
		ATMA_ASSERT(begin);
		ATMA_ASSERT(*begin != '\0');
		
		// in the middle of a byte sequence, iterate until we are no longer
		if ((*begin & 0xe0) == 0xc0) {
			while ((*begin & 0xe0) == 0xc0)
				++begin;
		}
		// at the leading byte of a sequence, use its information
		else {
			char marker = *begin;
			++begin;
			while (marker & 0x80) {
				marker <<= 1; ++begin;
			}
		}

		return begin;
	}
	
	inline auto utf8_next_char(char* begin) -> char*
	{
		return const_cast<char*>(utf8_next_char(const_cast<char const*>(begin)));
	}


	template <typename OT, typename IT>
	inline OT utf16_from_utf8(OT dest, IT begin, IT end)
	{
		static_assert(std::is_convertible<decltype(*begin), char const>::value, "type of input iterators must be char");

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
#endif // inclusion guard
//=====================================================================
