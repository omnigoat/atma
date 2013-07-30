#ifndef ATMA_UTF_UTF8_STRING_RANGE_HPP
#define ATMA_UTF_UTF8_STRING_RANGE_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
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
	//   This range is immutable, in that whilst we can change where we point
	//   to, we can not change the contents of what we point to.
	//
	//=====================================================================
	class utf8_string_range_t
	{
	public:
		typedef char value_t;
		typedef char const* iterator;

		utf8_string_range_t();
		utf8_string_range_t(char const* begin, char const* end);
		utf8_string_range_t(utf8_string_range_t const&);

		auto bytes() const -> uint32_t;

		auto begin() const -> iterator;
		auto end() const -> iterator;

	private:
		char const* begin_;
		char const* end_;
	};

	inline auto operator == (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return lhs.bytes() == rhs.bytes() && memcmp(lhs.begin(), rhs.begin(), lhs.bytes()) == 0;
	}

	inline auto operator != (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator < (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return
		  lhs.bytes() < rhs.bytes() || !(rhs.bytes() < lhs.bytes()) &&
		  memcmp(lhs.begin(), rhs.begin(), lhs.bytes()) < 0
		  ;
	}

	inline auto operator << (std::ostream& stream, utf8_string_range_t const& xs) -> std::ostream&
	{
		for (auto x : xs)
			stream.put(x);
		return stream;
	}




	//=====================================================================
	// utf8_string_range_t implementation
	//=====================================================================
	inline utf8_string_range_t::utf8_string_range_t()
		: begin_(), end_()
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(char const* begin, char const* end)
		: begin_(begin), end_(end)
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(utf8_string_range_t const& rhs)
		: begin_(rhs.begin_), end_(rhs.end_)
	{
	}

	inline auto utf8_string_range_t::bytes() const -> uint32_t
	{
		return end_ - begin_;
	}

	inline auto utf8_string_range_t::begin() const -> iterator
	{
		return begin_;
	}

	inline auto utf8_string_range_t::end() const -> iterator
	{
		return end_;
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
