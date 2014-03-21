#pragma once
//=====================================================================
#include <atma/unittest/unittest.hpp>
//=====================================================================
namespace atma { namespace unittest { namespace detail {

	//=====================================================================
	// entry point for verification
	//=====================================================================
	inline auto compile(tests_t::iterator const& begin, tests_t::iterator const& end, checks_t& output) -> void
	{
		for (tests_t::iterator i = begin; i != end; ++i) {
			test_t& test = **i;
			test.run(output);
		}
	}

} } }

