#pragma once

#include <atma/utf/algorithm.hpp>

#if 0
namespace atma
{
	template <typename BackingType>
	struct utf8_iterator_t
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = ptrdiff_t;
		using value_type        = utf8_char_t;
		using pointer           = utf8_char_t*;
		using reference         = utf8_char_t const&;

		utf8_iterator_t(utf8_iterator_t const&);

		auto operator = (utf8_iterator_t const&) -> utf8_iterator_t&;
		auto operator ++ () -> utf8_iterator_t&;
		auto operator ++ (int) -> utf8_iterator_t;
		auto operator -- () -> utf8_iterator_t&;
		auto operator -- (int) -> utf8_iterator_t;

		auto operator * () const -> reference;
		auto operator -> () const -> pointer;

	private:
		utf8_iterator_t(owner_t const*, char const*);

	private:
		char const* ptr_;
		mutable utf8_char_t ch_;

		friend auto operator == (utf8_string_t::utf8_iterator_t const&, utf8_string_t::utf8_iterator_t const&) -> bool;
	};


	inline auto operator == (utf8_string_t::utf8_iterator_t const&, utf8_string_t::utf8_iterator_t const&) -> bool;
	inline auto operator != (utf8_string_t::utf8_iterator_t const&, utf8_string_t::utf8_iterator_t const&) -> bool;
}

#endif