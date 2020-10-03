#include <atma/unit_test.hpp>

#include <atma/rope.hpp>

SCENARIO("rope can be constructed")
{
	GIVEN("")
	{
		THEN("")
		{
			atma::rope_t rope;
			rope.push_back("abcd", 4);
			rope.insert(0, "ef", 2);
		}
	}
}