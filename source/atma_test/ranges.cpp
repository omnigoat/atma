#include <atma/unit_test.hpp>

#include <atma/algorithm.hpp>
#include <atma/ranges/filter.hpp>
#include <atma/ranges/map.hpp>
#include <atma/ranges/zip.hpp>

import atma.vector;

// used below
struct is_3_t {
	template <typename X>
	bool operator ()(X x) const {
		return x == 3;
	}
} const is_3;



SCENARIO_OF("ranges/filter_t", "ranges can be filtered")
{
	auto is_even = [](int i) { return i % 2 == 0; };
	auto plus_10 = [](int i) { return i + 10; };
	auto is_gte3 = [](int i) { return i >= 3; };

	GIVEN("a prvalue vector of numbers")
	{
		using namespace atma;

		THEN("ownership is transferred")
		{
			auto result = atma::filter(is_even, atma::vector<int>{1, 2, 3, 4});
			static_assert(std::is_same_v<typename decltype(result)::storage_range_t, atma::vector<int>>);
		}

		THEN("basic filtering works")
		{
			auto result = atma::filter(is_even, atma::vector<int>{1, 2, 3, 4}) | atma::as_vector;
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
			auto result = atma::filter(is_even, numbers) | atma::as_vector;
			CHECK_WHOLE_VECTOR(numbers, 1, 2, 3, 4);
			CHECK_WHOLE_VECTOR(result, 2, 4);
		}

		THEN("lazy binding filtering works")
		{
			auto partial_filter = atma::filter(is_even);
			auto filtered = partial_filter(numbers);
			
			static_assert(atma::detail::is_filter_functor_v<decltype(partial_filter)>);

			// filtered must be referring to @numbers by const-reference
			static_assert(std::is_same_v<typename decltype(filtered)::storage_range_t, atma::vector<int> const&>);

			auto result = filtered | atma::as_vector;

			CHECK_WHOLE_VECTOR(result, 2, 4);
		}
	}

	GIVEN("a non-const lvalue vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};
		static_assert(std::ranges::range<decltype(numbers)>);

		THEN("cvrefness is preserved")
		{
			auto result = atma::filter(is_even, numbers);
			using iter = decltype(result.begin());
			static_assert(std::default_initializable<iter>);
			
			static_assert(std::move_constructible<iter>);
			static_assert(std::assignable_from<iter&, iter>);
			static_assert(std::swappable<iter>);

			static_assert(std::movable<iter>);

			static_assert(std::weakly_incrementable<iter>);

			auto rb = std::ranges::begin(result);
			auto re = std::ranges::end(result);
			static_assert(std::ranges::range<decltype(result)>, "filtered range not a std::ranges::range");
			static_assert(std::is_same_v<typename decltype(result)::storage_range_t, atma::vector<int>&>);
		}

		THEN("basic filtering works")
		{
			auto result = atma::filter(is_even, numbers) | atma::as_vector;
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



SCENARIO_OF("ranges/map", "ranges can be mapped")
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
			using ttt = std::ranges::range_reference_t<decltype(numbers)>;

			// range is a range
			using MR = decltype(atma::map(plus_10, numbers));
			static_assert(std::ranges::range<MR>);

			// iterator is an iterator
			using MRI = std::ranges::iterator_t<MR>;
			    static_assert(std::default_initializable<MRI>);
			    static_assert(std::movable<MRI>);
			  static_assert(std::weakly_incrementable<MRI>);
			static_assert(std::input_or_output_iterator<MRI>);

			auto numbers_plus10 = atma::map(plus_10, numbers) | atma::as_vector;
			
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
			auto numbers_plus10 = atma::map(plus_10, numbers) | atma::as_vector;
			CHECK_WHOLE_VECTOR(numbers_plus10, 11, 12, 13, 14);
		}
	}

	GIVEN("an rvalue vector of numbers")
	{
		using vector_type = atma::vector<int>;

		auto numbers = []() { return vector_type{1, 2, 3, 4}; };
		
		THEN("transfer of ownership occurs")
		{
			auto result = atma::map(plus_10, numbers());
			static_assert(std::is_same_v<typename decltype(result)::target_range_t, vector_type>);
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


SCENARIO_OF("ranges/zip", "ranges can be zipped")
{
	GIVEN("a const lvalue vector of numbers, and a const lvalue vector of strings")
	{
		atma::vector<int> numbers{1, 2, 3, 4};
		atma::vector<std::string> strings{"hello", "mr", "radio"};

		auto r = atma::zip(numbers, strings);
		using MR = decltype(r);

		// iterator is an iterator
		static_assert(std::default_initializable<MR::iterator>);
		static_assert(std::movable<MR::iterator>);
		static_assert(std::weakly_incrementable<MR::iterator>);
		static_assert(std::input_or_output_iterator<MR::iterator>);

		// range is a range
		static_assert(std::ranges::range<MR>);

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
