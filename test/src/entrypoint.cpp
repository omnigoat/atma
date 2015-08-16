#define CATCH_CONFIG_MAIN
#include <atma/unit_test.hpp>

#include <atma/vector.hpp>

SCENARIO("vectors can be constructed", "[vector]")
{
	GIVEN("a default-constructed vector")
	{
		atma::vector<int> v;

		CHECK(v.empty());
		CHECK(v.size() == 0);
		CHECK(v.capacity() == 0);
	}

	GIVEN("a vector constructed with size 4 default items")
	{
		atma::vector<int> v(4);

		CHECK(!v.empty());
		CHECK(v.size() == 4);
		CHECK(v.capacity() >= 4);
		CHECK(v[0] == 0); CHECK(v[1] == 0); CHECK(v[2] == 0); CHECK(v[3] == 0);
	}

	GIVEN("a vector constructed with {1, 2, 3, 4}")
	{
		atma::vector<int> v{1, 2, 3, 4};

		CHECK(!v.empty());
		CHECK(v.size() == 4);
		CHECK(v.capacity() >= 4);
		CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
	}
}

SCENARIO("vectors can be sized and resized", "[vector]")
{
	GIVEN("an empty vector")
	{
		atma::vector<int> v;

		CHECK(v.empty());
		CHECK(v.size() == 0);
		CHECK(v.capacity() >= 0);

		WHEN("it is resized")
		{
			v.resize(10);

			THEN("the size and capacity change") {
				CHECK(v.size() == 10);
				CHECK(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved")
		{
			v.reserve(10);

			THEN("the capacity changes but not the size") {
				CHECK(v.empty());
				CHECK(v.size() == 0);
				CHECK(v.capacity() >= 10);
			}
		}
	}
}
