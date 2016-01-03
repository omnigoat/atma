#pragma once

#include <atma/utf/utf8_string_range_header.hpp>
#include <atma/utf/utf8_string_header.hpp>

//=====================================================================
// utf8_string_range_t implementation
//=====================================================================
namespace atma
{
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
		: begin_(rhs.raw_begin()), end_(rhs.raw_end())
	{
	}

	inline utf8_string_range_t::utf8_string_range_t(utf8_string_t::const_iterator const& begin, utf8_string_t::const_iterator const& end)
		: begin_(*begin), end_(*end)
	{
	}

	inline auto utf8_string_range_t::raw_size() const -> size_t
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
}

