#pragma once

#include <atma/utf/utf8_string.hpp>
#include <atma/enable_if.hpp>

#include <algorithm>


namespace atma {

	// bam.
	typedef utf8_string_t string;

	template <typename T, typename = atma::enable_if<std::is_integral<T>, std::is_unsigned<T>>>
	inline auto to_string(T x, uint base = 10) -> atma::string
	{
		atma::string s;

		// do characters
		do {
			auto xb = x % base;

			if (xb > 9)
				s.push_back(xb - 10 + 'a');
			else
				s.push_back(xb + '0');

			x /= base;
		} while (x > 0);

		std::reverse(s.raw_begin(), s.raw_end());

		return s;
	}

}
