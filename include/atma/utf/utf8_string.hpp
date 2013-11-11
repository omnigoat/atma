#ifndef ATMA_UTF_UTF8_STRING
#define ATMA_UTF_UTF8_STRING
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_string_range.hpp>
#include <string>
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
		//explicit utf8_string_t(utf16_string_t const&);
		utf8_string_t(utf8_string_t const&);
		utf8_string_t(utf8_string_t&&);

		class iterator;

		auto empty() const -> bool { return chars_.empty(); }
		auto bytes() const -> uint32_t { return chars_.size(); }
		auto bytes_begin() const -> char const* { return &chars_[0]; }
		auto bytes_end() const -> char const* { return &chars_[0] + chars_.size(); }
		auto bytes_begin() -> char* { return &chars_[0]; }
		auto bytes_end() -> char* { return &chars_[0]; }

		// push back a single character is valid only for code-points < 128
		auto push_back(char c) -> void { 
			ATMA_ASSERT(c ^ 0x80);
			chars_.push_back(c);
			++char_count_;
		}

		auto operator += (utf8_string_t const& rhs) -> utf8_string_t&
		{
			chars_.insert(chars_.end(), rhs.bytes_begin(), rhs.bytes_end());
			return *this;
		}

	private:
		typedef std::vector<value_t> chars_t;
		chars_t chars_;
		uint32_t char_count_;


		friend class utf16_string_t;
		friend auto operator < (utf8_string_t const& lhs, utf8_string_t const& rhs) -> bool;
		friend auto operator + (utf8_string_t const& lhs, std::string const& rhs) -> utf8_string_t;
		friend auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t;
		friend auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&;
	};

	//=====================================================================
	// implementation
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

	inline auto operator + (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> utf8_string_t
	{
		auto result = lhs;
		result.chars_.insert(result.chars_.end(), rhs.begin(), rhs.end());
		return result;
	}

	inline auto operator << (std::ostream& out, utf8_string_t const& rhs) -> std::ostream&
	{
		for (auto const& x : rhs.chars_)
			out.put(x);
		//return out << rhs.chars_;
		return out;
	}


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
