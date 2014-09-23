#pragma once

#include <atma/utf/utf8_string_header.hpp>
#include <atma/utf/utf8_string_range_header.hpp>


namespace atma {

	inline utf8_string_t::utf8_string_t()
	{
		chars_.reserve(8);
		chars_.push_back('\0');
	}

	inline utf8_string_t::utf8_string_t(const_iterator const& begin, const_iterator const& end)
		: chars_((end->begin - begin->begin) + 1)
	{
		memcpy(&chars_.front(), begin->begin, (end->begin - begin->begin));
		chars_.back() = '\0';
	}

	inline utf8_string_t::utf8_string_t(char const* begin, char const* end)
		: chars_(begin, end)
	{
		chars_.push_back('\0');
	}

	inline utf8_string_t::utf8_string_t(char const* str)
	{
		ATMA_ASSERT(str != nullptr);

		for (char const* i = str; *i != '\0'; ++i) {
			chars_.push_back(*i);
		}

		chars_.push_back('\0');
	}

	inline utf8_string_t::utf8_string_t(utf8_string_range_t const& str)
	: chars_(str.begin(), str.end())
	{
		chars_.push_back('\0');
	}

#if 0
	utf8_string_t::utf8_string_t(const utf16_string_t& rhs)
	{
		// optimistically reserve one char per char16_t. this will work optimally
		// for english text, and will still totes work fine for anything else.
		chars_.reserve(rhs.char_count_);

		utf8_from_utf16(std::back_inserter(chars_), rhs.chars_.begin(), rhs.chars_.end());
	}
#endif

	inline utf8_string_t::utf8_string_t(utf8_string_t const& rhs)
	: chars_(rhs.chars_)
	{
	}

	inline utf8_string_t::utf8_string_t(utf8_string_t&& rhs)
	: chars_(std::move(rhs.chars_))
	{
	}

	inline auto utf8_string_t::empty() const -> bool
	{
		// null-terminator always present
		return chars_.size() == 1;
	}

	inline auto utf8_string_t::bytes() const -> size_t
	{
		// don't count null-terminator
		return chars_.size() - 1;
	}

	inline auto utf8_string_t::c_str() const -> char const*
	{
		return begin_raw();
	}

	inline auto utf8_string_t::begin_raw() const -> char const*
	{
		return &chars_[0];
	}

	inline auto utf8_string_t::end_raw() const -> char const*
	{
		return &chars_[0] + bytes();
	}

	inline auto utf8_string_t::begin_raw() -> char*
	{
		return &chars_[0];
	}

	inline auto utf8_string_t::end_raw() -> char*
	{
		return &chars_[0] + bytes();
	}

	inline auto utf8_string_t::begin() const -> const_iterator
	{
		return const_iterator(*this, begin_raw());
	}

	inline auto utf8_string_t::end() const -> const_iterator
	{
		return const_iterator(*this, end_raw());
	}

	inline auto utf8_string_t::begin() -> iterator
	{
		return iterator(*this, begin_raw());
	}

	inline auto utf8_string_t::end() -> iterator
	{
		return iterator(*this, end_raw());
	}

	inline auto utf8_string_t::push_back(char c) -> void {
		ATMA_ASSERT(c ^ 0x80);
		chars_.back() = c;
		chars_.push_back('\0');
	}

	inline auto utf8_string_t::push_back(utf8_char_t const& c) -> void
	{
		for (auto i = c.begin; i != c.end; ++i) {
			chars_.push_back(*i);
		}
	}

	inline auto utf8_string_t::operator += (utf8_string_t const& rhs) -> utf8_string_t&
	{
		chars_.insert(chars_.end() - 1, rhs.begin_raw(), rhs.end_raw());
		return *this;
	}

	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator == (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.bytes() == rhs.bytes() && ::strncmp(lhs.begin_raw(), rhs.begin_raw(), lhs.bytes()) == 0;
	}

	inline auto operator != (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator == (utf8_string_t const& lhs, char const* rhs) -> bool
	{
		return ::strcmp(lhs.begin_raw(), rhs) == 0;
	}

	inline auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return std::lexicographical_compare(lhs.chars_.begin(), lhs.chars_.end(), rhs.chars_.begin(), rhs.chars_.end());
	}

	inline auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end() - 1, rhs.chars_.begin(), rhs.chars_.end());
		return result;
	}

	inline auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end() - 1, rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&
	{
		if (!rhs.chars_.empty())
			out.write(&rhs.chars_[0], rhs.chars_.size() - 1);
		return out;
	}


	//=====================================================================
	// functions
	//=====================================================================
	inline auto strcpy(utf8_string_t const& dest, utf8_string_t const& src) -> void
	{
		ATMA_ASSERT(dest.bytes() >= src.bytes());

		auto i = dest.begin();
		for (auto const& x : src)
			*i++ = x;
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& whhere, char const* delims) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(delims);

		for (auto i = delims; *i; ++i)
			ATMA_ASSERT(utf8_char_is_ascii(*i));

		auto i2 = std::find_first_of(whhere, str.end(), delims, delims + strlen(delims));
		return {str, i2->begin};
	}

	inline auto find_first_of(utf8_string_t const& str, utf8_string_t::const_iterator const& i, char x) -> utf8_string_t::const_iterator
	{
		ATMA_ASSERT(utf8_char_is_ascii(x));

		auto i2 = std::find(i->begin, str.end_raw(), x);
		return{str, i2};
	}

	inline auto find_first_of(utf8_string_t const& str, char x) -> utf8_string_t::const_iterator
	{
		return find_first_of(str, str.begin(), x);
	}

}

