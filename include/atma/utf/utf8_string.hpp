#ifndef ATMA_UTF_UTF8_STRING_IMPLEMENTATION_HPP
#define ATMA_UTF_UTF8_STRING_IMPLEMENTATION_HPP
//=====================================================================
#include <atma/utf/utf8_string_header.hpp>
#include <atma/utf/utf8_string_range_header.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	//=====================================================================
	// utf8_string_range_t implementation
	//=====================================================================
	inline utf8_string_t::utf8_string_t()
	: char_count_()
	{
	}

	inline utf8_string_t::utf8_string_t(char const* begin, char const* end)
	: chars_(begin, end), char_count_()
	{
		for (char x : chars_) {
			if (is_utf8_leading_byte(x))
				++char_count_;
		}
	}

	inline utf8_string_t::utf8_string_t(char const* str)
	: char_count_()
	{
		ATMA_ASSERT(str != nullptr);

		for (char const* i = str; *i != '\0'; ++i) {
			chars_.push_back(*i);
			if (is_utf8_leading_byte(*i))
				++char_count_;
		}
	}

	inline utf8_string_t::utf8_string_t(utf8_string_range_t const& str)
	: chars_(str.begin(), str.end()), char_count_()
	{
		for (char x : chars_) {
			if (is_utf8_leading_byte(x))
				++char_count_;
		}
	}

#if 0
	utf8_string_t::utf8_string_t(const utf16_string_t& rhs)
	: char_count_()
	{
		// optimistically reserve one char per char16_t. this will work optimally
		// for english text, and will still totes work fine for anything else.
		chars_.reserve(rhs.char_count_);

		char_count_ = utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
	}
#endif

	inline utf8_string_t::utf8_string_t(const utf8_string_t& rhs)
	: chars_(rhs.chars_), char_count_(rhs.char_count_)
	{
	}

	inline utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
	: chars_(std::move(rhs.chars_)), char_count_(rhs.char_count_)
	{
	}
	
	inline auto utf8_string_t::begin() const -> const_iterator {
		return const_iterator(*this, bytes_begin());
	}

	inline auto utf8_string_t::end() const -> const_iterator {
		return const_iterator(*this, bytes_end());
	}

	inline auto utf8_string_t::begin() -> iterator {
		return iterator(*this, bytes_begin());
	}

	inline auto utf8_string_t::end() -> iterator {
		return iterator(*this, bytes_end());
	}


	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return std::lexicographical_compare(lhs.chars_.begin(), lhs.chars_.end(), rhs.chars_.begin(), rhs.chars_.end());
	}

	inline auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end(), rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end(), rhs.chars_.begin(), rhs.chars_.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end(), rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&
	{
		out.write(&rhs.chars_[0], rhs.chars_.size());
		return out;
	}


	//=====================================================================
	// functions
	//=====================================================================
	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& i, char x) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(is_ascii(x));

		auto i2 = std::find(i->begin, str.bytes_end(), x);
		return{str, i2};
	}

	inline auto find_first_of(utf8_string_t const& str, char x) -> utf8_string_t::const_iterator
	{
		return find_first_of(str, str.begin(), x);
	}


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
