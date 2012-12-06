#ifndef ATMA_STRING
#define ATMA_STRING
//=====================================================================
namespace atma {
//=====================================================================
	
	template <typename OT, typename IT>
	void utf8_from_utf16(OT dest, IT begin, IT end)
	{
		for (auto i = begin; i != end; ++i)
		{
			// we discard type inference to specify that utf16 strings are
			// comprised of char16_t
			char16_t const& x = *i;
			
			if (x < 0x80) {
				*dest++ = 0x7f & static_cast<char>(x);
			}
			else if (x < 0x800) {
				*dest++ = 0xa0 | (x >> 6);
				*dest++ = 0x80 | (x & 0x3f);
			}
			else if (x < 0xd800 || (0xdfff < x && x < 0x10000)) {
				*dest++ = 0xe0 | (x >> 12);
				*dest++ = 0x80 | ((x & 0x0fff) >> 6);
				*dest++ = 0x80 | (x & 0x3f);
			}
			// o damn, we're in surrogate land
			else {
				// if x is one char16_t, then xx is two
				char32_t xx = ((x - 0xd800) << 10) | (*++i - 0xdc00);
				xx += 0x10000;

				// all utf16 surrogates are 21 bits
				*dest++ = 0xf0 | (xx >> 18);
				*dest++ = 0x80 | ((xx & 0x3ffff) >> 12);
				*dest++ = 0x80 | ((xx & 0xfff) >> 6);
				*dest++ = 0x80 | (xx & 0x3f);
			}
		}
	}

	template <typename OT, typename IT>
	void utf16_from_utf8(OT dest, IT begin, IT end)
	{
		for (auto i = begin; i != end; ++i)
		{
			// *must* be char
			char const& x = *i;

			// top 4 bits set, 21 bits (surrogates!)
			if ((x & 0xf0) == 0xf0)
			{
				char32_t surrogate =
				  static_cast<char32_t>((x & 0x7) << 18) |
				  static_cast<char32_t>((*++i & 0x3f) << 12) |
				  static_cast<char32_t>((*++i & 0x3f) << 6 |
				  static_cast<char32_t>(*++i & 0x3f)
				  ;

				surrogate -= 0x10000;
				*dest++ = static_cast<char16_t>(surrogate >> 10) + 0xd800;
				*dest++ = static_cast<char16_t>(surrogate & 0xffa00) + 0xdc00;
			}
			else if ((x & 0xb0) == 0xb0) {
			}
			else {
				*dest++ = static_cast<char16_t>(x);
			}
		}
	}


	class utf8_string_t
	{
	public:
		typedef char value_t;

		utf8_string_t();
		utf8_string_t(const utf8_string_t&);
		utf8_string_t(utf8_string_t&&);

		//explicit utf8_string_t(value_t const* begin, value_t const* end);
		//explicit utf8_string_t(

		

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
	};


	utf8_string_t::utf8_string_t()
	{
	}

	utf8_string_t::utf8_string_t(const utf8_string_t& rhs)
	 : chars_(rhs.chars_)
	{
	}

	utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
	 : chars_( std::move(rhs.chars_) )
	{
	}


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

