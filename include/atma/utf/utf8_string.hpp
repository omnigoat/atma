#ifndef ATMA_UTF_UTF8_STRING
#define ATMA_UTF_UTF8_STRING
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	class utf8_string_t
	{
	public:
		typedef char value_t;

		utf8_string_t();
		utf8_string_t(char const* str);
		utf8_string_t(char const* str_begin, char const* str_end);
		explicit utf8_string_t(const utf16_string_t&);
		utf8_string_t(const utf8_string_t&);
		utf8_string_t(utf8_string_t&&);

		class iterator;

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
		uint32_t char_count_;

		friend class utf16_string_t;
	};

	//=====================================================================
	// implementation
	//=====================================================================
	utf8_string_t::utf8_string_t()
	: char_count_()
	{
	}

	utf8_string_t::utf8_string_t(char const* begin, char const* end)
	: chars_(begin, end), char_count_()
	{
		for (char x : chars_) {
			if (is_utf8_leading_byte(x))
				++char_count_;
		}
	}

	utf8_string_t::utf8_string_t(char const* str)
	: char_count_()
	{
		ATMA_ASSERT(str != nullptr);

		for (char const* i = str; *i != '\0'; ++i) {
			chars_.push_back(*i);
			if (is_utf8_leading_byte(*i))
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
	


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
