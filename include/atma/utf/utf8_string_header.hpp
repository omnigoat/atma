#pragma once

#include <atma/types.hpp>

#include <atma/utf/utf8_char.hpp>

#include <ranges>
#include <string>
#include <iterator>

#if 0
// forward declares
namespace atma::detail
{
	template <bool> struct basic_utf8_string_range_t;
}

namespace atma
{
	struct utf8_char_t;
	struct utf8_string_t;
	using utf8_string_range_t = detail::basic_utf8_string_range_t<false>;
	using utf8_const_range_t = detail::basic_utf8_string_range_t<true>;
}
#endif
