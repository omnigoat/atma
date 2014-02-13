#ifndef ATMA_UTF_UTF8_STRING_RANGE_HPP
#define ATMA_UTF_UTF8_STRING_RANGE_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_string_header.hpp>
#include <algorithm>
//=====================================================================
namespace atma {
//=====================================================================
	

	//=====================================================================
	// utf8_string_range_t
	// ---------------------
	//   A string-range that does not allocate any memory, but instead points
	//   to external, contiguous memory that is expected to be valid for the
	//   lifetime of the string-range.
	//
	//   This range is immutable.
	//
	//=====================================================================
	class utf8_string_range_t
	{
	public:
		typedef char value_t;
		typedef char const* iterator;

		utf8_string_range_t();
		utf8_string_range_t(char const*);
		utf8_string_range_t(char const* begin, char const* end);
		utf8_string_range_t(utf8_string_t::const_iterator const&, utf8_string_t::const_iterator const&);
		utf8_string_range_t(utf8_string_range_t const&);
		utf8_string_range_t(utf8_string_t const&);

		auto bytes() const -> size_t;
		
		auto begin() const -> iterator;
		auto end() const -> iterator;

	private:
		char const* begin_;
		char const* end_;
	};




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator == (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return lhs.bytes() == rhs.bytes() && memcmp(lhs.begin(), rhs.begin(), lhs.bytes()) == 0;
	}

	inline auto operator == (utf8_string_range_t const& lhs, char const* rhs) -> bool
	{
		return strncmp(lhs.begin(), rhs, lhs.bytes()) == 0;
	}

	inline auto operator != (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator < (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		auto cmp = ::strncmp(
			lhs.begin(), rhs.begin(),
			std::min(lhs.bytes(), rhs.bytes()));

		return
			cmp < 0 || (!(0 < cmp) && (
			lhs.bytes() < rhs.bytes()));
	}

	inline auto operator << (std::ostream& stream, utf8_string_range_t const& xs) -> std::ostream&
	{
		for (auto x : xs)
			stream.put(x);
		return stream;
	}



	//=====================================================================
	// functions
	//=====================================================================
	inline auto strncmp(utf8_string_range_t const& lhs, char const* str, uint32 n) -> uint32
	{
		return std::strncmp(lhs.begin(), str, n);
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
