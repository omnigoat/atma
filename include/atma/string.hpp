#ifndef ATMA_STRING
#define ATMA_STRING
//=====================================================================
namespace atma {
//=====================================================================
	
	template <typename OT, typename IT>
	void utf8_from_utf16(OT dest, IT begin, IT end)
	{
		// for simplicity, we'll subsume the utf16 character into our
		// 32-bit 
		//char32_t x = 0;

		for (auto i = begin; i != end; ++i)
		{
			char16_t const& x = *i;
			//x = utf16_char;

			// if utf16_char is actually a head of a surrogate pair, then
			// we'll simply
			//if (0xd800 <= utf16_char && utf16_char <= 0xdfff) {
			//	x = 0xd800 + (utf16_char >> 6)
			//}


			if (x < 0x80) {
				*dest++ = 0x7f & static_cast<char>(x);
			}
			else if (x < 0x800) {
				*dest++ = 0xa0 | (x >> 6);
				*dest++ = 0x80 | (x & 0x3f);
			}
			else if (x < 0xd800) {
				*dest++ = 0xe0 | (x >> 12);
				*dest++ = 0x80 | ((x & 0x0fff) >> 6);
				*dest++ = 0x80 | (x & 0x3f);
			}
			// o damn, we're in surrogate land
			else if (x < 0xe000) {
				// if x is one char16_t, then xx is two
				char32_t xx = ((x - 0xd800) << 10) | (*++i - 0xdc00);
				xx += 0x10000;

				if (xx < 200000) {
					*dest++ = 0xf8 | (xx >> 18);
					*dest++ = 0x80 | ((xx & 0x3ffff) >> 12);
					*dest++ = 0x80 | ((xx & 0xfff) >> 6);
					*dest++ = 0x80 | (xx & 0x3f);
				}
			}
		}
	}


	//class utf8_string_t
	//{
	//public:
	//	typedef char value_t;

	//	utf8_string_t();
	//	utf8_string_t(const utf8_string_t&);
	//	utf8_string_t(utf8_string_t&&);

	//	//explicit utf8_string_t(value_t const* begin, value_t const* end);
	//	//explicit utf8_string_t(

	//	

	//private:
	//	typedef std::vector<value_t> chars_t;
	//	chars_t chars_;
	//};


	//utf8_string_t::utf8_string_t(value_t const* begin, value_t const* end)
	//{
	//	chars_.assign(begin, end);
	//}


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

