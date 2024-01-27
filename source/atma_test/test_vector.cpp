
#include <atma/unit_test.hpp>
#include <atma/algorithm.hpp>

import atma.vector;
import atma.memory;

using canary_t = atma::unit_test::canary_t;


SCENARIO_OF("vector", "vectors can be constructed")
{
	GIVEN("a default-constructed vector")
	{
		using vector_type = atma::vector<int>;

		vector_type v;

		static_assert(std::is_nothrow_default_constructible_v<typename vector_type::allocator_type>);
		static_assert(std::is_nothrow_constructible_v<vector_type>);

		THEN("the vector is empty")
		{
			CHECK(v.empty());
			CHECK(v.size() == 0);
			CHECK(v.capacity() == 0);
		}
	}

	GIVEN_CANARY("a vector constructed with size of four")
	{
		atma::vector<canary_t> v(4);

		THEN("I have four default-constructed elements")
		{
			CHECK(!v.empty());
			CHECK(v.size() == 4);
			CHECK(v.capacity() >= 4);
		}
	}
	THEN_CANARY
	{
		// default-construct in v
		C.default_constructor(1);
		C.default_constructor(2);
		C.default_constructor(3);
		C.default_constructor(4);
		// v destructs
		C.destructor(1);
		C.destructor(2);
		C.destructor(3);
		C.destructor(4);
	}

	GIVEN_CANARY("a vector constructed with size 4 copy-constructed items")
	{
		atma::vector<canary_t> v(4, canary_t{13});

		THEN("there are four duplicates")
		{
			CHECK(!v.empty());
			CHECK(v.capacity() >= 4);
			CHECK_WHOLE_VECTOR(v, 13, 13, 13, 13);
		}
	}
	THEN_CANARY
	{
		// temporary constructs
		C.direct_constructor(1);
		// four copies
		C.copy_constructor(2);
		C.copy_constructor(3);
		C.copy_constructor(4);
		C.copy_constructor(5);
		// temporary destructs
		C.destructor(1);
		// v destructs
		C.destructor(2);
		C.destructor(3);
		C.destructor(4);
		C.destructor(5);
	}

	GIVEN_CANARY("a vector constructed with {1, 2, 3, 4}")
	{
		atma::vector<canary_t> v{1, 2, 3, 4};
		
		THEN("we have those elements present")
		{
			CHECK(!v.empty());
			CHECK(v.capacity() >= 4);
			CHECK_WHOLE_VECTOR(v, 1, 2, 3, 4);
		}
	}
	THEN_CANARY
	{
		// direct-construct initializer-list
		C.direct_constructor(1);
		C.direct_constructor(2);
		C.direct_constructor(3);
		C.direct_constructor(4);
		// copy-construct into v
		C.copy_constructor(5);
		C.copy_constructor(6);
		C.copy_constructor(7);
		C.copy_constructor(8);
		// initializer-list destructs
		C.destructor(4);
		C.destructor(3);
		C.destructor(2);
		C.destructor(1);
		// v destructs
		C.destructor(5);
		C.destructor(6);
		C.destructor(7);
		C.destructor(8);
	}



	GIVEN_CANARY("a vector copy-constructed from {1, 2, 3, 4}")
	{
		atma::vector<canary_t> v1{1, 2, 3, 4};
		atma::vector<canary_t> v2{v1};

		THEN("we have equivalent {1,2,3,4} & {1,2,3,4}")
		{
			CHECK(!v2.empty());
			CHECK(v2.capacity() >= 4);
			CHECK_WHOLE_VECTOR(v2, 1,2,3,4);
			CHECK(v2 == v1);
		}
	}
	THEN_CANARY
	{
		// construction of temporaries
		C.direct_constructor(1);
		C.direct_constructor(2);
		C.direct_constructor(3);
		C.direct_constructor(4);
		// copy-construct into v1 (not the title case)
		C.copy_constructor(5);
		C.copy_constructor(6);
		C.copy_constructor(7);
		C.copy_constructor(8);
		// destruct temporaries
		C.destructor(4);
		C.destructor(3);
		C.destructor(2);
		C.destructor(1);
		// copy-construct into v2
		C.copy_constructor(9);
		C.copy_constructor(10);
		C.copy_constructor(11);
		C.copy_constructor(12);
		// v2 destructs
		C.destructor(9);
		C.destructor(10);
		C.destructor(11);
		C.destructor(12);
		// v1 destructs
		C.destructor(5);
		C.destructor(6);
		C.destructor(7);
		C.destructor(8);
	}



	GIVEN_CANARY("a move-constructed vector")
	{
		atma::vector<canary_t> v1{1, 2, 3, 4};
		atma::vector<canary_t> v2{std::move(v1)};

		THEN("original vector is empty")
		{
			CHECK(v1.empty());
			CHECK(v1.size() == 0);
			CHECK(v1.capacity() == 0);
		}

		THEN("new vector initialized correctly")
		{
			CHECK(!v2.empty());
			CHECK(v2.capacity() >= 4);
			CHECK_WHOLE_VECTOR(v2, 1,2,3,4);
		}
	}
	THEN_CANARY
	{
		// construction of temporaries
		C.direct_constructor(1);
		C.direct_constructor(2);
		C.direct_constructor(3);
		C.direct_constructor(4);
		// temporaries copy-constructed into v1
		C.copy_constructor(5);
		C.copy_constructor(6);
		C.copy_constructor(7);
		C.copy_constructor(8);
		// destruct temporaries
		C.destructor(4);
		C.destructor(3);
		C.destructor(2);
		C.destructor(1);
		// move-constructor of vector just swaps buffers, nothing happens
		//
		// v1 destructs, now empty
		//
		// v2 destructs
		C.destructor(5);
		C.destructor(6);
		C.destructor(7);
		C.destructor(8);
	}

}

SCENARIO_OF("vector", "vectors can be inserted into")
{
	GIVEN("a vector constructed with {1, 2, 3, 4}")
	{
		atma::vector<int> n{1, 2, 3, 4};

		WHEN("number 0 is inserted at the front")
		{
			n.insert(n.begin(), 0);

			THEN("{0, 1, 2, 3, 4}")
			{
				CHECK_WHOLE_VECTOR(n, 0, 1, 2, 3, 4);
			}
		}

		WHEN("number 5 is inserted at the back")
		{
			n.insert(n.end(), 5);

			THEN("{1, 2, 3, 4, 5}")
			{
				CHECK_WHOLE_VECTOR(n, 1, 2, 3, 4, 5);
			}
		}

		WHEN("number 17 is inserter at index 2")
		{
			n.insert(n.begin() + 2, 17);

			THEN("looks like: {1, 2, 17, 3, 4}")
			{
				CHECK_WHOLE_VECTOR(n, 1, 2, 17, 3, 4);
			}
		}
	}
}


SCENARIO_OF("vector", "vectors can be sized and resized")
{
	GIVEN("an empty vector")
	{
		atma::vector<int> v;

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

SCENARIO_OF("vector", "vectors can be assigned")
{
	GIVEN("an empty vector 'v' and vector 'v2'={1,2,3,4}")
	{
		atma::vector<int> v;
		atma::vector<int> v2{1, 2, 3, 4};

		WHEN("v is assigned v2")
		{
			v = v2;

			THEN("an exact copy is made")
			{
				CHECK(!v.empty());
				CHECK_WHOLE_VECTOR(v, 1,2,3,4);
				CHECK(v == v2);
			}
		}

		WHEN("v is move-assigned v2")
		{
			v = std::move(v2);

			THEN("v has the elements")
			{
				CHECK(!v.empty());
				CHECK_WHOLE_VECTOR(v, 1, 2, 3, 4);
			}

			THEN("v2 is reduced to nothing")
			{
				CHECK(v2.empty());
				CHECK(v2.capacity() == 0);
			}
		}
	}
}


SCENARIO_OF("vector", "vector::insert is called")
{
	GIVEN("an empty vector 'v' and vector 'v2'={1,2,3,4}")
	{
		atma::vector<int> v;
		atma::vector<int> v2{1, 2, 3, 4};

		WHEN("v.insert(v2.begin, v2.end) is called")
		{
			v.insert(v.end(), v2.begin(), v2.end());

			THEN("an exact copy is made")
			{
				CHECK(!v.empty());
				CHECK_WHOLE_VECTOR(v, 1, 2, 3, 4);
				CHECK(v == v2);
			}
		}
	}

	GIVEN("an empty vector 'v' and vector 'v2'={'henry', 'theodore', 'marcie', 'rachael'}")
	{
		atma::vector<std::string> v{"timothy", "maria"};
		atma::vector<std::string> v2{"henry", "theodore", "marcie", "rachael"};

		//atma::vector<int> numbers{1, 2, 3, 4};
		//numbers.erase(numbers.begin());

		WHEN("v.insert(v2.begin, v2.end) is called")
		{
			v.insert(v.begin() + 1, v2.begin(), v2.end());

			THEN("an exact copy is made")
			{
				CHECK(!v.empty());
				CHECK_WHOLE_VECTOR(v, "timothy", "henry", "theodore", "marcie", "rachael", "maria");
			}
		}
	}
}
