//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_UNITTEST_COMPILE_HPP
#define ATMA_UNITTEST_COMPILE_HPP
//=====================================================================
#include <atma/unittest/unittest.hpp>
//=====================================================================
namespace atma {
namespace unittest {
namespace detail {
//=====================================================================
	
	//=====================================================================
	// entry point for verification
	//=====================================================================
	inline void compile(tests_t::iterator begin, tests_t::iterator end, checks_t& output)
	{
		for (tests_t::iterator i = begin; i != end; ++i) {
			test_t& test = **i;
			test.run(output);
		}
	}
	
//=====================================================================
} // namespace detail
} // namespace unittest
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

