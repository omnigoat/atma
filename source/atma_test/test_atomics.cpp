#include <atma/unit_test.hpp>

import atma.atomic;
import atma.types;


//
// I mean, how do we even test that these sort of functions work
//
SCENARIO("atomics!")
{
	GIVEN("an aligned 16-bit uint16_t with a value known to us")
	{
		uint16_t blah = 4;
		WHEN("we call atma::atomic_load")
		{
			auto r = atma::atomic_load(&blah);
			r = atma::atomic_add(&blah, uint16_t{4}, atma::memory_order::relaxed);
			THEN("the loaded value equals the variable")
			{
				CHECK(r == blah);
			}
		}
	}
}

