#include <atma/unit_test.hpp>

import atma.atomic;


SCENARIO("atomics!")
{
	GIVEN("things")
	{
		uint16_t blah = 4;
		atma::atomic_load(&blah);
	}
}

