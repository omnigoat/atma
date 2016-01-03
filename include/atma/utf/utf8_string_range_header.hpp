#pragma once

#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_string_header.hpp>
#include <atma/assert.hpp>
#include <algorithm>

namespace atma
{
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
	struct utf8_string_range_t
	{
		typedef char value_t;
		typedef char const* iterator;

		utf8_string_range_t();
		explicit utf8_string_range_t(char const*);
		explicit utf8_string_range_t(utf8_string_t const&);
		utf8_string_range_t(char const* begin, char const* end);
		utf8_string_range_t(utf8_string_t::const_iterator const&, utf8_string_t::const_iterator const&);
		utf8_string_range_t(utf8_string_range_t const&);

		auto raw_size() const -> size_t;
		auto empty() const -> bool;

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
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.begin(), rhs.begin(), lhs.raw_size()) == 0;
	}

	inline auto operator != (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator == (utf8_string_range_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.begin(), rhs.raw_begin(), lhs.raw_size()) == 0;
	}

	inline auto operator == (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return lhs.raw_size() == rhs.raw_size() && memcmp(lhs.raw_begin(), rhs.begin(), lhs.raw_size()) == 0;
	}

	inline auto operator != (utf8_string_range_t const& lhs, utf8_string_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator != (utf8_string_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		return !operator == (lhs, rhs);
	}

	inline auto operator == (utf8_string_range_t const& lhs, char const* rhs) -> bool
	{
		return strncmp(lhs.begin(), rhs, lhs.raw_size()) == 0;
	}


	inline auto operator < (utf8_string_range_t const& lhs, utf8_string_range_t const& rhs) -> bool
	{
		auto cmp = ::strncmp(
			lhs.begin(), rhs.begin(),
			std::min(lhs.raw_size(), rhs.raw_size()));

		return
			cmp < 0 || (!(0 < cmp) && (
			lhs.raw_size() < rhs.raw_size()));
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

	inline auto rebase_string_range(utf8_string_t const& rebase, utf8_string_t const& oldbase, utf8_string_range_t const& range) -> utf8_string_range_t
	{
		return{
			rebase.raw_begin() + (range.begin() - oldbase.raw_begin()),
			rebase.raw_begin() + (range.end() - oldbase.raw_begin())
		};
	}
}
