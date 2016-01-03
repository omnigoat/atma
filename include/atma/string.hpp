#pragma once

#include <atma/utf/utf8_string.hpp>
#include <atma/utf/utf8_string_range.hpp>

#include <atma/enable_if.hpp>
#include <atma/vector.hpp>

#include <algorithm>


namespace atma {

	// bam.
	using string = utf8_string_t;
	using string_range_t = utf8_string_range_t;


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

	



	inline auto split(atma::string const& s, char const* delims) -> atma::vector<atma::string>
	{
		auto begin = s.begin();
		auto end = s.end();

		atma::vector<atma::string> result;

		auto si = begin;
		for (auto i = begin, pi = begin; i != end; ++i)
		{
			for (auto const* d = delims; *d; utf8_char_advance(d))
			{
				//if (utf8_char_equality(*i, d))
				//{
				//	result.push_back(atma::string(si, pi));
				//	si = i;
				//	break;
				//}
			}

			pi = i;
		}
	}

}
