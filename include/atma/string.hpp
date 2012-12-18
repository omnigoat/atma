#ifndef ATMA_STRING
#define ATMA_STRING
//=====================================================================
#include <vector>
#include <iterator>
#include <algorithm>
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	template <typename OT, typename IT>
	unsigned int utf8_from_utf16(OT dest, IT begin, IT end)
	{
		unsigned int characters = 0;

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

			++characters;
		}

		return characters;
	}

	template <typename OT, typename IT>
	unsigned int utf16_from_utf8(OT dest, IT begin, IT end)
	{
		unsigned int characters = 0;

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

			++characters;
		}

		return characters;
	}

	class utf8_string_t;
	class utf16_string_t;

	class utf8_string_t
	{
	public:
		typedef char value_t;

		utf8_string_t();
		utf8_string_t(char const* str);
		utf8_string_t(char const* str_begin, char const* str_end);
		utf8_string_t(const utf16_string_t&);
		utf8_string_t(const utf8_string_t&);
		utf8_string_t(utf8_string_t&&);

		class iterator;

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
		unsigned int char_count_;

		friend class utf16_string_t;
	};

	class utf16_string_t
	{
	public:
		typedef char16_t value_t;

		utf16_string_t();
		utf16_string_t(const utf16_string_t&);
		utf16_string_t(const utf8_string_t&);
		utf16_string_t(utf16_string_t&&);

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
		unsigned int char_count_;

		friend class utf8_string_t;
	};

	class utf8_stream_iterator
	{
	public:
		utf8_stream_iterator();
		utf8_stream_iterator(char const* stream);
		
		utf8_stream_iterator& operator ++();
		
		char const* begin() const;
		char const* end() const;
		unsigned int byte_count() const;

	private:
		void update_character_end();

		char const* stream_;
		char const* character_end_;
	};

	inline bool operator == (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs) {
		return lhs.byte_count() == rhs.byte_count() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}

	inline bool operator != (const utf8_stream_iterator& lhs, const utf8_stream_iterator& rhs) {
		return lhs.byte_count() != rhs.byte_count() || !std::equal(lhs.begin(), lhs.end(), rhs.end());
	}


	typedef utf8_string_t string;


	utf8_string_t::utf8_string_t()
	 : char_count_()
	{
	}

	utf8_string_t::utf8_string_t(char const* str_begin, char const* str_end)
	 : char_count_( std::distance(str_begin, str_end) )
	{
		//chars_.resize(char_count_);
		//std::copy(str_begin, str_end, chars.begin());
	}

	utf8_string_t::utf8_string_t(char const* str)
	 : char_count_()
	{
		ATMA_ASSERT(str != nullptr);

		for (utf8_stream_iterator i{str}; i != utf8_stream_iterator(); ++i) {
			chars_.insert(chars_.end(), i.begin(), i.end());
			++char_count_;
		}
	}

	utf8_string_t::utf8_string_t(const utf16_string_t& rhs)
	 : char_count_()
	{
		// optimistically reserve one char per char16_t. this will work optimally
		// for english text, and will still totes work fine for anything else.
		chars_.reserve(rhs.char_count_);

		char_count_ = utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
	}

	utf8_string_t::utf8_string_t(const utf8_string_t& rhs)
	 : chars_(rhs.chars_), char_count_(rhs.char_count_)
	{
	}

	utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
	 : chars_(std::move(rhs.chars_)), char_count_(rhs.char_count_)
	{
	}

	


	utf8_stream_iterator::utf8_stream_iterator()
	 : utf8_stream_iterator("\0")
	{
	}

	utf8_stream_iterator::utf8_stream_iterator(char const* stream)
	 : stream_(stream), character_end_()
	{
		// if we've been placed in the middle of a sequence, then
		// find the beginning of the next character
		while (stream_ && (*stream_ & 0xc0) == 0x80) {
			++stream_;
		}

		update_character_end();
	}

	char const* utf8_stream_iterator::begin() const {
		return stream_;
	}

	char const* utf8_stream_iterator::end() const {
		return character_end_;
	}

	unsigned int utf8_stream_iterator::byte_count() const {
		return std::distance(stream_, character_end_);
	}

	utf8_stream_iterator& utf8_stream_iterator::operator ++ ()
	{
		ATMA_ASSERT(*stream_ != '\0');

		stream_ = character_end_;
		update_character_end();

		return *this;
	}

	void utf8_stream_iterator::update_character_end()
	{
		// continuation bytes are not allowed!
		ATMA_ASSERT((*stream_ & 0xa0) != 0x80);

		character_end_ = stream_;
		if (!(*stream_ & 0x80)) {
			++character_end_;
		}
		else {
			char marker = *stream_;
			while (marker & 0x80) {
				marker <<= 1; ++character_end_;
			}
		}
	}


	//=====================================================================
	// utf16_string_t
	//=====================================================================
	utf16_string_t::utf16_string_t()
	 : char_count_()
	{
	}

	utf16_string_t::utf16_string_t(const utf8_string_t& rhs)
	 : char_count_()
	{
		chars_.reserve(rhs.char_count_);
		char_count_ = utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
		chars_.shrink_to_fit();
	}

	utf16_string_t::utf16_string_t(const utf16_string_t& rhs)
	 : chars_(rhs.chars_), char_count_(rhs.char_count_)
	{
	}

	utf16_string_t::utf16_string_t(utf16_string_t&& rhs)
	 : chars_(std::move(rhs.chars_)), char_count_(rhs.char_count_)
	{
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

