#include <atma/unit_test.hpp>

import atma.rope;

#include <iostream>

SCENARIO("rope can be constructed" * doctest::skip())
{
	int k = atma::detail::rope_branching_factor;

	GIVEN("")
	{
		THEN("")
		{
			atma::rope_t rope;
			rope.push_back("ab", 2);
			rope.push_back("cd", 2);
			rope.insert(3, "xy", 2);
			rope.insert(2, "fg", 2);

			std::cout << rope << std::endl;
		}
	}
}
