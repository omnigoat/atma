#include <atma/unit_test.hpp>

#include <atma/algorithm.hpp>
#include <atma/vector.hpp>
#include <atma/meta.hpp>

// used below
struct is_3_t {
	template <typename X>
	bool operator ()(X x) const {
		return x == 3;
	}
} const is_3;


template <typename T, typename U,
	CONCEPT_REQUIRES_((atma::concepts::SameC<T, U>()))
>
auto operator % (T&& lhs, U&& rhs)
{
	return "chicken";
}


SCENARIO("ranges can be filtered", "[ranges/filter_t]")
{
	auto is_even = [](int i) { return i % 2 == 0; };
	auto plus_10 = [](int i) { return i + 10; };
	auto is_gte3 = [](int i) { return i >= 3; };

	GIVEN("a prvalue vector of numbers")
	{
		struct dragon_t {};
		struct knight_t {};

		auto r = dragon_t{} % dragon_t{};

		THEN("ownership is transferred")
		{
			auto result = atma::filter(is_even, atma::vector<int>{1, 2, 3, 4});
			static_assert(std::is_same_v<typename decltype(result)::storage_range_t, atma::vector<int>>);
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
			static_assert(std::is_same_v<typename decltype(result)::storage_range_t, atma::vector<int> const&>);
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

			static_assert(std::is_same_v<typename decltype(filtered)::storage_range_t, atma::vector<int> const&>);
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}
	}

	GIVEN("a non-const lvalue vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("cvrefness is preserved")
		{
			auto result = atma::filter(is_even, numbers);
			static_assert(std::is_same_v<typename decltype(result)::storage_range_t, atma::vector<int>&>);
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
			auto result
				= numbers
				| atma::filter(is_gte3)
				| atma::filter(is_even)
				| atma::as_vector
				;

			//atma::as_vector |  (atma::filter(is_even) | atma::filter(is_gte3, numbers));
			//auto result2 = atma::as_vector | (atma::filter(is_even, atma::filter(is_gte3, numbers)) );


			CHECK_WHOLE_VECTOR(result, 4);
			//CHECK_WHOLE_VECTOR(result2, 4);
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



SCENARIO("ranges can be mapped", "[ranges/map]")
{
	auto plus_10 = [](int i) { return i + 10; };
	auto mul_2 = [](int x) { return x * 2; };

	struct noncopyable_negate
	{
		noncopyable_negate() = default;
		noncopyable_negate(noncopyable_negate const&) = delete;
		noncopyable_negate(noncopyable_negate&&) = default;

		int operator ()(int x) const { return -x; }
	};


	GIVEN("a const lvalue vector of numbers")
	{
		auto const numbers = atma::vector<int>{1, 2, 3, 4};

		THEN("basic mapping works")
		{
			auto numbers_plus10 = atma::as_vector | atma::map(plus_10, numbers);
			
			CHECK_WHOLE_VECTOR(numbers_plus10, 11, 12, 13, 14);
		}

		THEN("chaining is great")
		{
			auto result = numbers | atma::map(plus_10) | atma::map(mul_2) | atma::as_vector;
			CHECK_WHOLE_VECTOR(result, 22, 24, 26, 28);
		}

		THEN("for-each works I guess?")
		{
			atma::vector<int> compar{22, 24, 26, 28};
			auto b = compar.begin();

			numbers 
				| atma::map(plus_10)
				| atma::map(mul_2)
				| atma::for_each([&b](auto&& x) {
					CHECK(x == *b++);
				});
		}

		THEN("functions can be non-copyable")
		{
			noncopyable_negate f{};
			
			auto transformer_lv = atma::map(f);
			auto transformer_rv = atma::map(noncopyable_negate{});

			auto transformed_lv_lv = numbers | transformer_lv;
			auto transformed_lv_rv = numbers | transformer_rv;
			auto transformed_rv_lv = numbers | atma::map(f);
			auto transformed_rv_rv = numbers | atma::map(noncopyable_negate{});

			static_assert(std::is_same_v<typename decltype(transformed_lv_lv)::storage_functor_t, noncopyable_negate&>);
			static_assert(std::is_same_v<typename decltype(transformed_lv_rv)::storage_functor_t, noncopyable_negate&>);
			static_assert(std::is_same_v<typename decltype(transformed_rv_lv)::storage_functor_t, noncopyable_negate&>);
			static_assert(std::is_same_v<typename decltype(transformed_rv_rv)::storage_functor_t, noncopyable_negate>);

			CHECK_WHOLE_VECTOR(transformed_lv_lv | atma::as_vector, -1, -2, -3, -4);
			CHECK_WHOLE_VECTOR(transformed_lv_rv | atma::as_vector, -1, -2, -3, -4);
			CHECK_WHOLE_VECTOR(transformed_rv_lv | atma::as_vector, -1, -2, -3, -4);
			CHECK_WHOLE_VECTOR(transformed_rv_rv | atma::as_vector, -1, -2, -3, -4);
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
			static_assert(std::is_same_v<typename decltype(result)::target_range_t, atma::vector<int>>);
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


SCENARIO("ranges can be zipped", "[ranges/zip]")
{
	GIVEN("a const lvalue vector of numbers, and a const lvalue vector of strings")
	{
		atma::vector<int> numbers{1, 2, 3, 4};
		atma::vector<std::string> strings{"hello", "mr", "radio"};

		{
			int count = 0;
			for (auto&& [n, s] : atma::zip(numbers, strings))
				++count;
			CHECK(count == 3);
		}

		{
			int count = 0;
			auto number_was_even = [](auto&& n, auto&& s) { return n % 2 == 0; };
			for (auto&&[n, s] : atma::zip(numbers, strings) | atma::filter(number_was_even))
				++count;
			CHECK(count == 1);
		}

		
	}
}
