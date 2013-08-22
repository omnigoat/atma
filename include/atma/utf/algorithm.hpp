#ifndef ATMA_UTF_ALGORITHM
#define ATMA_UTF_ALGORITHM
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	inline bool is_utf8_leading_byte(char const c) {
		return (c & 0xc0) != 0xa0;
	}

	inline char const* utf8_next_char(char const* begin)
	{
		ATMA_ASSERT(begin);
		ATMA_ASSERT(*begin != '\0');
		
		// in the middle of a byte sequence, iterate until we are no longer
		if ((*begin & 0xe0) == 0xc0) {
			while ((*begin & 0xe0) == 0xc0)
				++begin;
		}
		// at the leading byte of a sequence, use its information
		else {
			char marker = *begin;
			++begin;
			while (marker & 0x80) {
				marker <<= 1; ++begin;
			}
		}

		return begin;
	}
	


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================