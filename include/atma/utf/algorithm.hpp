#pragma once
//=====================================================================
#include <atma/utf/utf8_char.hpp>

#include <atma/types.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
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
