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

	GIVEN("a vector constructed with size 4 copy-constructed items")
	{
		atma::vector<int> v(4, 13);

		CHECK(!v.empty());
		CHECK(v.size() == 4);
		CHECK(v.capacity() >= 4);
		CHECK(v[0] == 13); CHECK(v[1] == 13); CHECK(v[2] == 13); CHECK(v[3] == 13);
	}

	GIVEN("a vector constructed with {1, 2, 3, 4}")
	{
		atma::vector<int> v{1, 2, 3, 4};

		CHECK(!v.empty());
		CHECK(v.size() == 4);
		CHECK(v.capacity() >= 4);
		CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
	}

	GIVEN("a copy-constructed vector")
	{
		atma::vector<int> g{1, 2, 3, 4};
		atma::vector<int> v{g};

		CHECK(!v.empty());
		CHECK(v.size() == 4);
		CHECK(v.capacity() >= 4);
		CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
		CHECK(v == g);
	}

	GIVEN("a move-constructed vector")
	{
		atma::vector<int> g{1, 2, 3, 4};
		atma::vector<int> v{std::move(g)};

		THEN("origin vector is empty")
		{
			CHECK(g.empty());
			CHECK(g.size() == 0);
			CHECK(g.capacity() == 0);
		}

		AND_THEN("constructed vector equal")
		{
			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
		}
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

			THEN("the size and capacity change")
			{
				CHECK(v.size() == 10);
				CHECK(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved")
		{
			v.reserve(10);

			THEN("the capacity changes but not the size")
			{
				CHECK(v.empty());
				CHECK(v.size() == 0);
				CHECK(v.capacity() >= 10);
			}
		}

		WHEN("more capacity is reserved, then shrink_to_fit")
		{
			v.reserve(10);
			v.shrink_to_fit();

			THEN("the capacity is zero")
			{
				CHECK(v.empty());
				CHECK(v.size() == 0);
				CHECK(v.capacity() == 0);
			}
		}
	}
}
