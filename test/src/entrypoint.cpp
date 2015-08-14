#define CATCH_CONFIG_MAIN
#include <atma/unit_test.hpp>

#include <atma/vector.hpp>


SCENARIO("vectors can be sized and resized", "[vector]") {

	GIVEN("an empty vector")
	{
		atma::vector<int> v;

		REQUIRE(v.empty());
		REQUIRE(v.size() == 0);
		REQUIRE(v.capacity() >= 0);

		WHEN("the size is increased")
		{
			v.resize(10);

			THEN("the size and capacity change") {
				REQUIRE(v.size() == 10);
				REQUIRE(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved")
		{
			v.reserve(10);

			THEN("the capacity changes but not the size") {
				REQUIRE(v.size() == 0);
				REQUIRE(v.capacity() >= 10);
			}
		}
	}
}
