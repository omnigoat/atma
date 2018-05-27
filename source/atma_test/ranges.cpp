#include <atma/unit_test.hpp>

#include <atma/algorithm/filter.hpp>
#include <atma/algorithm/map.hpp>
#include <atma/lockfree_queue.hpp>

#include <atma/vector.hpp>

constexpr struct inc_t {
	constexpr auto operator ()(int x) const -> int { return x + 1; }
} const inc;

constexpr struct dec_t {
	auto operator ()(std::string x) const -> int { return 1; }
	auto operator ()(int x) const -> int { return x - 1; }
} const dec;

constexpr struct square_t {
	constexpr auto operator ()(int x) const -> int { return x * x; }
} const square;

constexpr struct mult_t {
	constexpr auto operator ()(int x, int y) const -> int { return x * y; }
} const mult;

struct thing
{
	void operator ()(int) {}
	void operator ()(std::string) {}
};

template <bool A, bool... R>
struct all_of_t
{
	constexpr static bool const value = A && all_of_t<R...>::value;
};

template <bool A>
struct all_of_t<A>
{
	constexpr static bool const value = A;
};

int times2(int x) { return x * 2; }



struct is_3_t {
	template <typename X>
	bool operator ()(X x) const {
		return x == 3;
	}
} const is_3;

template <typename T> decltype(auto) iden(T t) { return t; }

SCENARIO("ranges can be filtered", "[ranges/filter_t]")
{
	GIVEN("a vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};
		auto const cnumbers = numbers;

		auto is_even = [](int i) { return i % 2 == 0; };
		auto plus_10 = [](int i) { return i + 10; };
		auto is_gte3 = [](int i) { return i >= 3; };

		THEN("filtering mutable (no change) works")
		{
			auto result = atma::filter(is_even, numbers);
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
			CHECK(numbers[0] == 1); CHECK(numbers[1] == 2); CHECK(numbers[2] == 3); CHECK(numbers[3] == 4);
		}

		THEN("filtering mutable (changing) works")
		{
			auto lnumbers = numbers;
			auto result = atma::filter(is_even, lnumbers);
			for (auto&& x : result)
				x += 10;

			CHECK(lnumbers[0] == 1); CHECK(lnumbers[1] == 12); CHECK(lnumbers[2] == 3); CHECK(lnumbers[3] == 14);
		}

		THEN("filtering const works")
		{
			auto result = atma::filter(is_even, cnumbers);
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
			CHECK(numbers[0] == 1); CHECK(numbers[1] == 2); CHECK(numbers[2] == 3); CHECK(numbers[3] == 4);
		}

		THEN("filtering moved data-structures works")
		{
			auto N = numbers;
			auto R = atma::filter(is_even, std::move(N));
			auto RV = atma::vector<int>{R.begin(), R.end()};

			CHECK(N.empty());
			CHECK(RV.size() == 2);
			CHECK(RV[0] == 2);
			CHECK(RV[1] == 4);
		}

		THEN("lazy binding filtering works")
		{
			auto filter2 = atma::filter(is_even);
			auto result = filter2(numbers);
			auto resultv = atma::vector<int>{result.begin(), result.end()};

			CHECK(resultv.size() == 2);
			CHECK(resultv[0] == 2);
			CHECK(resultv[1] == 4);
		}

#if 0
		THEN("chaining filters is fine and dandy")
		{
			auto rgte3 = atma::filter(is_even) % atma::filter(is_even) % atma::filter(is_gte3, numbers);
			auto result = atma::vector<int>{rgte3.begin(), rgte3.end()};

			CHECK(result.size() == 1);
			CHECK(result[0] == 4);
		}

		THEN("chaining filters for rvalues is fine and dandy")
		{
			auto N = numbers;
			auto fgte3 = atma::filter(is_gte3);
			auto rgte3 = atma::filter(is_even) % fgte3;
			auto f3 = atma::filter(is_even) % rgte3;
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
	GIVEN("a vector of numbers")
	{
		auto numbers = atma::vector<int>{1, 2, 3, 4};

		auto plus_10 = [](int i) { return i + 10; };
		

		THEN("we can map plus 10")
		{
			auto yay10 = atma::map(plus_10, numbers);
			std::vector<int> resultv{yay10.begin(), yay10.end()};
			CHECK_WHOLE_VECTOR(resultv, 11, 12, 13, 14);
		}
	}
}