#ifndef ATMA_UTF_UTF8_STRING_RANGE_IMPLEMENTATION_HPP
#define ATMA_UTF_UTF8_STRING_RANGE_IMPLEMENTATION_HPP
//=====================================================================
#include <atma/utf/utf8_string_range_header.hpp>
#include <atma/utf/utf8_string_header.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	//=====================================================================
	// utf8_string_range_t implementation
	//=====================================================================
	inline utf8_string_range_t::utf8_string_range_t()
		: begin_(), end_()
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(char const* begin)
		: begin_(begin), end_(begin)
	{
		while (*end_)
			++end_;
	}

	inline utf8_string_range_t::utf8_string_range_t(char const* begin, char const* end)
		: begin_(begin), end_(end)
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(utf8_string_range_t const& rhs)
		: begin_(rhs.begin_), end_(rhs.end_)
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(utf8_string_t const& rhs)
		: begin_(rhs.begin_raw()), end_(rhs.end_raw())
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end)
		: begin_(begin->begin), end_(end->begin)
	{
	}

	inline auto utf8_string_range_t::bytes() const -> size_t
	{
		return end_ - begin_;
	}

	inline auto utf8_string_range_t::empty() const -> bool
	{
		return begin_ == end_;
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
	// functions
	//=====================================================================
	inline auto rebase_string_range(utf8_string_t const& rebase, utf8_string_t const& oldbase, utf8_string_range_t const& range) -> utf8_string_range_t
	{
		return {
			rebase.begin_raw() + (range.begin() - oldbase.begin_raw()),
			rebase.begin_raw() + (range.end() - oldbase.begin_raw())
		};
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
