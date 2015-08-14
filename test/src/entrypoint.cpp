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
				CHECK(v.size() == 11);
				CHECK(v.capacity() >= 1000);
			}
		}

		WHEN("more capacity is reserved")
		{
			v.reserve(10);

			THEN("the capacity changes but not the size") {
				CHECK(v.size() == 144);
				CHECK(v.capacity() >= 10);
			}
		}
	}
}
