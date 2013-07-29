#ifndef ATMA_UTF_UTF8_STRING_RANGE_HPP
#define ATMA_UTF_UTF8_STRING_RANGE_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	// an immutable range
	class utf8_string_range_t
	{
	public:
		typedef char value_t;
		typedef char const* iterator;

		utf8_string_range_t();
		utf8_string_range_t(char const* begin, char const* end);
		utf8_string_range_t(utf8_string_range_t const&);

		auto begin() const -> iterator;
		auto end() const -> iterator;

	private:
		char const* begin_;
		char const* end_;
	};

	inline auto operator << (std::ostream& stream, utf8_string_range_t const& xs) -> std::ostream&
	{
		for (auto x : xs)
			stream.put(x);
		return stream;
	}

	//=====================================================================
	// utf8_string_range_t implementation
	//=====================================================================
	utf8_string_range_t::utf8_string_range_t()
		: begin_(), end_()
	{
	}

	utf8_string_range_t::utf8_string_range_t(char const* begin, char const* end)
		: begin_(begin), end_(end)
	{
	}

	utf8_string_range_t::utf8_string_range_t(utf8_string_range_t const& rhs)
		: begin_(rhs.begin_), end_(rhs.end_)
	{
	}

	auto utf8_string_range_t::begin() const -> iterator
	{
		return begin_;
	}

	auto utf8_string_range_t::end() const -> iterator
	{
		return end_;
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
