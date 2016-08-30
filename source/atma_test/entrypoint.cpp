#define CATCH_CONFIG_MAIN
#include <atma/unit_test.hpp>

#include <atma/vector.hpp>

using canary_t = atma::unit_test::canary_t;

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
		CANARY_SWITCH_SCOPE("default-constructed")
		{
			atma::vector<canary_t> v(4);

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
		}

		CHECK_CANARY_SCOPE("default-constructed",
			// default-construct in v
			{1, canary_t::default_constructor},
			{2, canary_t::default_constructor},
			{3, canary_t::default_constructor},
			{4, canary_t::default_constructor},
			// v destructs
			{1, canary_t::destructor},
			{2, canary_t::destructor},
			{3, canary_t::destructor},
			{4, canary_t::destructor},
		);
	}

	GIVEN("a vector constructed with size 4 copy-constructed items")
	{	
		CANARY_SWITCH_SCOPE("copy-constructed")
		{
			atma::vector<canary_t> v(4, canary_t{13});

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0].payload == 13); CHECK(v[1].payload == 13); CHECK(v[2].payload == 13); CHECK(v[3].payload == 13);
		}

		CHECK_CANARY_SCOPE("copy-constructed",
			// temporary constructs
			{1, canary_t::direct_constructor},
			// four copies
			{2, canary_t::copy_constructor},
			{3, canary_t::copy_constructor},
			{4, canary_t::copy_constructor},
			{5, canary_t::copy_constructor},
			// temporary destructs
			{1, canary_t::destructor},
			// v destructs
			{2, canary_t::destructor},
			{3, canary_t::destructor},
			{4, canary_t::destructor},
			{5, canary_t::destructor},
		);
	}

	GIVEN("a vector constructed with {1, 2, 3, 4}")
	{
		CANARY_SWITCH_SCOPE("initializer-list")
		{
			atma::vector<canary_t> v{1, 2, 3, 4};

			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
			CHECK(v[0] == 1); CHECK(v[1] == 2); CHECK(v[2] == 3); CHECK(v[3] == 4);
		}

		CHECK_CANARY_SCOPE("initializer-list",

			// initializer-list
			CANARY_CC_ORDER(
				{1, canary_t::direct_constructor},
				{2, canary_t::direct_constructor},
				{3, canary_t::direct_constructor},
				{4, canary_t::direct_constructor}),

			// copy-construct into v
			{5, canary_t::copy_constructor},
			{6, canary_t::copy_constructor},
			{7, canary_t::copy_constructor},
			{8, canary_t::copy_constructor},

			// initializer-list destructs
			CANARY_CC_ORDER(
				{4, canary_t::destructor},
				{3, canary_t::destructor},
				{2, canary_t::destructor},
				{1, canary_t::destructor}),

			// v destructs
			{5, canary_t::destructor},
			{6, canary_t::destructor},
			{7, canary_t::destructor},
			{8, canary_t::destructor},
		);
	}

	GIVEN("a copy-constructed vector")
	{
		CANARY_SWITCH_SCOPE("copy-construct-vector")
		{
			atma::vector<canary_t> v1{1, 2, 3, 4};
			atma::vector<canary_t> v2{v1};

			CHECK(!v2.empty());
			CHECK(v2.size() == 4);
			CHECK(v2.capacity() >= 4);
			CHECK(v2[0].payload == 1); CHECK(v2[1].payload == 2); CHECK(v2[2].payload == 3); CHECK(v2[3].payload == 4);
			CHECK(v2 == v1);
		}

		CHECK_CANARY_SCOPE("copy-construct-vector",

			// construction of temporaries
			CANARY_CC_ORDER(
				{1, canary_t::direct_constructor},
				{2, canary_t::direct_constructor},
				{3, canary_t::direct_constructor},
				{4, canary_t::direct_constructor}),

			// copy-construct into v1
			{5, canary_t::copy_constructor},
			{6, canary_t::copy_constructor},
			{7, canary_t::copy_constructor},
			{8, canary_t::copy_constructor},

			// destruct temporaries
			CANARY_CC_ORDER(
				{4, canary_t::destructor},
				{3, canary_t::destructor},
				{2, canary_t::destructor},
				{1, canary_t::destructor}),

			// copy-construct into v2
			{9, canary_t::copy_constructor},
			{10, canary_t::copy_constructor},
			{11, canary_t::copy_constructor},
			{12, canary_t::copy_constructor},

			// v2 destructs
			{9, canary_t::destructor},
			{10, canary_t::destructor},
			{11, canary_t::destructor},
			{12, canary_t::destructor},

			// v1 destructs
			{5, canary_t::destructor},
			{6, canary_t::destructor},
			{7, canary_t::destructor},
			{8, canary_t::destructor},
		);
	}

	GIVEN("a move-constructed vector")
	{
		CANARY_SWITCH_SCOPE("vector::move-constructor")
		{
			atma::vector<canary_t> v1{1, 2, 3, 4};
			atma::vector<canary_t> v2{std::move(v1)};

			// original vector empty
			CHECK(v1.empty());
			CHECK(v1.size() == 0);
			CHECK(v1.capacity() == 0);

			// new vector initialized correctly
			CHECK(!v2.empty());
			CHECK(v2.size() == 4);
			CHECK(v2.capacity() >= 4);
			CHECK(v2[0] == 1); CHECK(v2[1] == 2); CHECK(v2[2] == 3); CHECK(v2[3] == 4);
		}

		CHECK_CANARY_SCOPE("vector::move-constructor",

			// construction of temporaries
			CANARY_CC_ORDER(
				{1, canary_t::direct_constructor},
				{2, canary_t::direct_constructor},
				{3, canary_t::direct_constructor},
				{4, canary_t::direct_constructor}),

			// copy-construct into v1
			{5, canary_t::copy_constructor},
			{6, canary_t::copy_constructor},
			{7, canary_t::copy_constructor},
			{8, canary_t::copy_constructor},

			// destruct temporaries
			CANARY_CC_ORDER(
				{4, canary_t::destructor},
				{3, canary_t::destructor},
				{2, canary_t::destructor},
				{1, canary_t::destructor}),

			// move-constructor of vector just swaps buffers, nothing happens

			// v1 destructs, now empty

			// v2 destructs
			{5, canary_t::destructor},
			{6, canary_t::destructor},
			{7, canary_t::destructor},
			{8, canary_t::destructor},
		);
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

SCENARIO("vectors can be assigned", "[vector]")
{
	GIVEN("an empty vector and vector of four components")
	{
		atma::vector<int> v;
		atma::vector<int> v2{1, 2, 3, 4};

		WHEN("v is assigned v2")
		{
			v = v2;

			THEN("a copy is made")
			{
				CHECK(!v.empty());
				CHECK(v.size() == 4);
				CHECK(v == v2);
			}
		}

		WHEN("v is move-assigned v2")
		{
			v = std::move(v2);

			THEN("v2 is reduced to nothing")
			{
				CHECK(!v.empty());
				CHECK(v.size() == 4);
				CHECK(v2.empty());
				CHECK(v2.capacity() == 0);

				atma::vector<int> t{1, 2, 3, 4};
				CHECK(v == t);
			}
		}
	}
}
