#include <atma/unit_test.hpp>

#include <atma/algorithm.hpp>
#include <atma/vector.hpp>


// used below
struct is_3_t {
	template <typename X>
	bool operator ()(X x) const {
		return x == 3;
	}
} const is_3;


SCENARIO("ranges can be filtered", "[ranges/filter_t]")
{
	auto is_even = [](int i) { return i % 2 == 0; };
	auto plus_10 = [](int i) { return i + 10; };
	auto is_gte3 = [](int i) { return i >= 3; };

	GIVEN("a prvalue vector of numbers")
	{
		THEN("ownership is transferred")
		{
			auto result = atma::filter(is_even, atma::vector<int>{1, 2, 3, 4});
			static_assert(std::is_same_v<typename decltype(result)::storage_container_t, atma::vector<int>>);
		}

		THEN("basic filtering works")
		{
			auto result = atma::as_vector | atma::filter(is_even, atma::vector<int>{1, 2, 3, 4});
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}
	}

	GIVEN("an const lvalue vector of numbers")
	{
		auto const numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("cvrefness is preserved")
		{
			auto result = atma::filter(is_even, numbers);
			static_assert(std::is_same_v<typename decltype(result)::storage_container_t, atma::vector<int> const&>);
		}

		THEN("basic filtering works")
		{
			auto result = atma::as_vector | atma::filter(is_even, numbers);
			CHECK_WHOLE_VECTOR(numbers, 1, 2, 3, 4);
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}

		THEN("lazy binding filtering works")
		{
			auto partial_filter = atma::filter(is_even);
			auto filtered = partial_filter(numbers);
			auto result = atma::as_vector | filtered;

			static_assert(std::is_same_v<typename decltype(filtered)::storage_container_t, atma::vector<int> const&>);
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}
	}

	GIVEN("a non-const lvalue vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("cvrefness is preserved")
		{
			auto result = atma::filter(is_even, numbers);
			static_assert(std::is_same_v<typename decltype(result)::storage_container_t, atma::vector<int>&>);
		}

		THEN("basic filtering works")
		{
			auto result = atma::as_vector | atma::filter(is_even, numbers);
			CHECK_WHOLE_VECTOR(numbers, 1, 2, 3, 4);
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}

		THEN("mutating filtered elements mutates the original elements")
		{
			for (auto&& x : atma::filter(is_even, numbers))
				x += 10;
			CHECK_WHOLE_VECTOR(numbers, 1, 12, 3, 14);
		}

		THEN("chaining filters is fine and dandy")
		{
			auto rgte3 = atma::filter(is_even) | atma::filter(is_gte3, numbers);
			auto result = atma::vector<int>{rgte3.begin(), rgte3.end()};
			
			auto rgte3v2 = atma::filter(is_even, atma::filter(is_gte3, numbers));
			auto resultv2 = atma::vector<int>{rgte3v2.begin(), rgte3v2.end()};

			CHECK_WHOLE_VECTOR(result, 4);
			CHECK_WHOLE_VECTOR(resultv2, 4);
		}

#if 0
		THEN("chaining filters for rvalues is fine and dandy")
		{
			auto N = numbers;
			auto fgte3 = atma::filter(is_gte3);
			auto rgte3 = atma::filter(is_even) | fgte3;
			auto f3 = atma::filter(is_even) | rgte3;
			auto R = f3(std::move(N));
			auto result = atma::vector<int>{R.begin(), R.end()};

			CHECK(N.empty());
			CHECK(result.size() == 1);
			CHECK(result[0] == 4);
		}
#endif

		THEN("filtering using an interesting predicate compiles")
		{
			auto R = atma::filter(is_3, numbers);
			R.begin(); R.end();
		}
	}
}



SCENARIO("ranges can be mapped", "[ranges/map_t]")
{
	auto plus_10 = [](int i) { return i + 10; };

	GIVEN("a const lvalue vector of numbers")
	{
		auto const numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("basic mapping works")
		{
			auto numbers_plus10 = atma::as_vector | atma::map(plus_10, numbers);
			static_assert(std::is_same_v<decltype(numbers_plus10), atma::vector<int>>);
			static_assert(std::is_same_v<typename decltype(numbers_plus10)::value_type, int>);
			CHECK_WHOLE_VECTOR(numbers_plus10, 11, 12, 13, 14);
		}
	}

	GIVEN("a non-const lvalue vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("we can map plus10")
		{
			auto numbers_plus10 = atma::as_vector | atma::map(plus_10, numbers);
			CHECK_WHOLE_VECTOR(numbers_plus10, 11, 12, 13, 14);
		}

		

	}

	GIVEN("an rvalue vector of numbers")
	{
		auto numbers = []() { return atma::vector<int>{1, 2, 3, 4}; };

		THEN("transfer of ownership occurs")
		{
			auto result = atma::map(plus_10, numbers());
			static_assert(std::is_same_v<typename std::remove_reference_t<decltype(result)>::storage_container_t, atma::vector<int>>);
		}
	}

	GIVEN("an lvalue vector of dragons")
	{
		struct dragon_t { std::string name; int age; };

		auto dragons = atma::vector<dragon_t>{
			{"henry", 21},
			{"oliver", 30},
			{"josephine", 28}
		};


		THEN("a mapping function that returns references behaves appropritately")
		{
			auto dragon_name = [](auto&& x) -> std::string& { return x.name; };

			auto result = atma::map(dragon_name, dragons);
			static_assert(std::is_same_v<decltype(*result.begin()), std::string&>);
		}
	}

}
