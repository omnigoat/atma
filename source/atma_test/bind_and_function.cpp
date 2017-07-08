#include <atma/unit_test.hpp>

#include <atma/bind.hpp>
#include <atma/function.hpp>
#include <atma/thread/engine.hpp>

int add(int a, int b) { return a + b; }

int square(int x) { return x * x; }
auto squareL = [](int x) { return x * x; };

struct mathing_t { 
	int halve(int x) { return x / 2; }
	int chalve(int x) const { return x / 2; }
	int operator ()(int x) { return x; }
};

struct tm_t
{
	template <typename A>
	std::enable_if_t<std::is_same_v<A, int>, int>
	operator ()(A a) const { return a; }

	template <typename A>
	std::enable_if_t<std::is_same<A, float>::value, float>
	operator ()(A a) const { return a * 2.f; }
};

int what() { return 4; }

SCENARIO("bind works with various things", "[bind]")
{
	GIVEN("a binding to a free-function, and one to am equivalent non-closure lambda")
	{
		auto a = atma::bind(&square, arg1);
		auto b = atma::bind(squareL, arg1);

		THEN("they equate")
		{
			CHECK(4 == a(2));
			CHECK(16 == b(4));
			CHECK(a(3) == b(3));
		}
	}

	GIVEN("binds of various flavours")
	{
		//atma::thread::inplace_engine_t<true> lulz{4096};

		// regular function & binding a binding
		
		// mumber function
		mathing_t m;
		mathing_t const m2;
		auto b2v1 = atma::bind(&mathing_t::halve, arg2, arg1);
		auto b2v2 = atma::bind(&mathing_t::halve, &m, arg1);
		auto b2v3 = atma::bind(&mathing_t::halve, std::ref(m), arg1);
		auto b2v4 = atma::bind(&mathing_t::chalve, std::cref(m2), 16);
		auto b2v5 = atma::bind(&mathing_t::halve, mathing_t(), arg1);
		auto b2v6 = atma::curry(&mathing_t::halve, &m);
		auto b2v7 = atma::curry(&mathing_t::halve, m, 16);

		// lambda object
		auto L = [](int x) { return x + 1; };
		auto b3v1 = atma::bind(L, 9);
		auto b3v2 = atma::curry(L);

		// atma::function object
		auto F = atma::function<int(char, int, float)>{[](char x, int y, float z){ return x * y + (int)z; }};
		auto F2 = atma::function<int(char, int, float)>{F};
		auto b4v1 = atma::curry(F);
		auto b4v2 = atma::bind(F, arg2, arg3, arg1);

		// composition object with well-defined arguments
		auto b5v1 = atma::curry(b2v6 % b2v5);
		auto b5v2 = atma::bind(b5v1 % b2v2, arg1);
		auto b5v3 = b5v1 % b2v7;
		
		// composition object with a templated `operator ()`
		tm_t tm;
		auto b6 = atma::bind(tm, arg1);

		THEN("we can make a basic function")
		{
			char buf[128];
			auto tb = atma::bind(&square, 4);
			atma::basic_generic_function_t<8, atma::functor_storage_t::heap, int()> tf{tb};
			atma::basic_generic_function_t<8, atma::functor_storage_t::heap, int()> tf2;
			tf2 = tf;

			auto r = tf();
			auto r2 = tf2();
			auto cr = r + r2;
		}

		THEN("all b2s match each other")
		{
			CHECK(8 == b2v1(16, &m));
			CHECK(8 == b2v2(16));
			CHECK(8 == b2v3(16));
			CHECK(8 == b2v4());
			CHECK(8 == b2v5(16));
			CHECK(8 == b2v6(16));
			CHECK(8 == b2v7());
		}

		THEN("all b3s match each other")
		{
			CHECK(10 == b3v1());
			CHECK(10 == b3v2(9));
		}

		THEN("b4s behave")
		{
			CHECK(b4v1(1, 2, 3.f) == 5);
			CHECK(b4v2(4.f, 5, 6) == 34);
		}

		THEN("b5s play nice")
		{
			CHECK(b5v1(12) == 3);
			CHECK(b5v2(16) == 2);
		}

		THEN("b6 works at all. accomplishment.")
		{
			CHECK(b6(4) == 4);
			CHECK(b6(4.f) == 8.f);
		}
	}
}

SCENARIO("functions can be constructed")
{
	GIVEN("a default-constructed basic_function")
	{
		atma::function<int(int, int)> f;

		THEN("it is empty")
		{
			CHECK(!(bool)f);
			CHECK(f.target<int(*)(int, int)>() == nullptr);
		}
	}

	GIVEN("a directly-constructed basic_function")
	{
		atma::function<int(int, int)> f{&add};

		THEN("it is filled with our function")
		{
			CHECK((bool)f);
			auto t = f.target<int(*)(int, int)>();
			CHECK(t != nullptr);
			CHECK(*t == &add);
		}
	}
}
