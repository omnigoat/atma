#include <atma/unit_test.hpp>

#include <atma/rope.hpp>

SCENARIO("rope can be constructed")
{
	GIVEN("")
	{
		THEN("")
		{
			atma::rope_t rope;
			rope.push_back("abcde", 5);
			rope.insert(2, "fg", 2);
		}
	}
}