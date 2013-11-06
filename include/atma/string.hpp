#ifndef ATMA_STRING
#define ATMA_STRING
//=====================================================================
#include <vector>
#include <iterator>
#include <algorithm>
//=====================================================================
#include <atma/assert.hpp>
#include <atma/utf/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/utf/utf8_string_range.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	

	// bam.
	typedef utf8_string_t string;


	auto to_string(uint32_t n) -> string
	{
		string s;
		while (n) {
			s.push_back(n % 10 + 32);
			n /= 10;
		}

		std::reverse(s.begin(), s.end());

		return s;
	}



//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

