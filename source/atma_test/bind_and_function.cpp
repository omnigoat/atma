#include <atma/unit_test.hpp>

#include <atma/function.hpp>
#include <atma/thread/engine.hpp>

#include <functional>
#include <type_traits>

import atma.bind;

int mul(int a, int b) { return a * b; }
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


SCENARIO_OF("bind", "bind works with various things")
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

		auto thetest = atma::curry(b2v6);
		thetest(16);

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

		// lambda object
		auto L = [](int x) { return x + 1; };
		auto b3v1 = atma::bind(L, 9);
		auto b3v2 = atma::curry(L);

		// atma::function object
		auto F = atma::function<int(char, int, float)>([](char x, int y, float z){ return x * y + (int)z; });
		auto F2 = atma::function<int(char, int, float)>{F};
		auto b4v1 = atma::curry(F);
		auto b4v2 = atma::bind(F, arg2, arg3, arg1);

#if 0
		// composition object with well-defined arguments
		auto b5v1 = atma::curry(b2v6 | b2v5);
		auto b5v2 = atma::bind(b5v1 | b2v2, arg1);
		auto b5v3 = b5v1 | b2v7;
		
		// composition object with a templated `operator ()`
		tm_t tm;
		auto b6 = atma::bind(tm, arg1);

		THEN("we can make a basic function")
		{
			auto tb = atma::bind(&square, 4);
			atma::basic_generic_function_t<16, atma::functor_storage_t::heap, int()> tf{tb};
			atma::basic_generic_function_t<16, atma::functor_storage_t::heap, int()> tf2;
			tf2 = tf;

			auto r = tf();
			auto r2 = tf2();
			auto cr = r + r2;

			CHECK(cr == 32);
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
			CHECK(b4v1(char(1), 2, 3.f) == 5);
			CHECK(b4v2(4.f, char(5), 6) == 34);
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
#endif
	}

}

SCENARIO_OF("atma::function", "functions can be constructed")
{
	GIVEN("a default-constructed function")
	{
		atma::function<int(int, int)> f;

		THEN("function is empty")
		{
			CHECK(!(bool)f);
			CHECK(f.target<int(*)(int, int)>() == nullptr);
		}
	}

	GIVEN("a default-constructed external_function")
	{
		atma::external_function<int(int, int)> f;

		THEN("function is empty")
		{
			CHECK(!(bool)f);
			CHECK(f.target<int(*)(int, int)>() == nullptr);
		}
	}

	GIVEN("a default-constructed relative_function")
	{
		atma::relative_function<int(int, int)> f;

		THEN("function is empty")
		{
			CHECK(!(bool)f);
			CHECK(f.target<int(*)(int, int)>() == nullptr);
		}
	}

	GIVEN("a directly-constructed function")
	{
		atma::function<int(int, int)> f{&add};

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<int(*)(int, int)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<int(*)(int, int)>();
			CHECK(t != nullptr);
			CHECK(*t == &add);
		}
	}

	GIVEN("a directly-constructed external function with SFO")
	{
		char buf[128];
		atma::external_function<int(int, int)> f(&add, (void*)buf);

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<int(*)(int, int)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<int(*)(int, int)>();
			CHECK(t != nullptr);
			CHECK(*t == &add);
		}

		THEN("external buffer size is zero")
		{
			CHECK(f.external_buffer_size() == 0);
		}
	}

	GIVEN("a directly-constructed external function *without* SFO (copy-constructor)")
	{
		char buf[128];
		auto b = atma::bind(&add, arg1, arg2);
		atma::basic_external_function_t<16, int(int, int)> f(b, (void*)buf);

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<decltype(b)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<decltype(b)>();
			CHECK(t != nullptr);
		}

		THEN("external buffer size is *not* zero")
		{
			CHECK(f.external_buffer_size() != 0);
		}
	}

	GIVEN("a directly-constructed relative function *without* SFO (move-constructor)")
	{
		char buf[128];
		auto b = atma::bind(&add, arg1, arg2);
		atma::basic_relative_function_t<16, int(int, int)> f(atma::bind(&add, arg1, arg2), (void*)buf);

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<decltype(b)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<decltype(b)>();
			CHECK(t != nullptr);
		}

		THEN("external buffer size is *not* zero")
		{
			CHECK(f.external_buffer_size() != 0);
		}
	}

	GIVEN("a directly-constructed relative function *without* SFO (copy-constructor)")
	{
		char buf[128];
		auto b = atma::bind(&add, arg1, arg2);
		atma::basic_relative_function_t<16, int(int, int)> f(b, (void*)buf);

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<decltype(b)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<decltype(b)>();
			CHECK(t != nullptr);
		}

		THEN("relative buffer size is *not* zero")
		{
			CHECK(f.external_buffer_size() != 0);
		}
	}

	GIVEN("a directly-constructed relative function *without* SFO (move-constructor)")
	{
		char buf[128];
		auto b = atma::bind(&add, arg1, arg2);
		atma::basic_relative_function_t<16, int(int, int)> f(std::move(b), (void*)buf);

		THEN("it is not empty")
		{
			CHECK((bool)f);
			CHECK(f.target<decltype(b)>() != nullptr);
		}

		THEN("it is filled with our function")
		{
			auto t = f.target<decltype(b)>();
			CHECK(t != nullptr);
		}

		THEN("relative buffer size is *not* zero")
		{
			CHECK(f.external_buffer_size() != 0);
		}
	}

	GIVEN("a copy-constructed function")
	{
		atma::function<int(int, int)> f{&add};
		atma::function<int(int, int)> g{f};

		THEN("it is not empty")
		{
			CHECK((bool)g);
			CHECK(g.target<int(*)(int, int)>() != nullptr);
		}

		THEN("it is targeting the original function")
		{
			auto t = g.target<int(*)(int, int)>();
			CHECK(t != nullptr);
			CHECK(*t == &add);
		}
	}

	GIVEN("a move-constructed function")
	{
		atma::function<int(int, int)> f{&add};
		atma::function<int(int, int)> g{std::move(f)};

		THEN("it is not empty")
		{
			CHECK((bool)g);
			CHECK(g.target<int(*)(int, int)>() != nullptr);
		}

		THEN("it is targeting the original function")
		{
			auto t = g.target<int(*)(int, int)>();
			CHECK(t != nullptr);
			CHECK(*t == &add);
		}
	}

	GIVEN("something")
	{
		int resulty = 0;
		std::function<void()> stdf = [&resulty] { resulty = 4; };

		uint32_t u32a, u32b;
		uint64_t u64a, u64this;

		auto L = [u64this, u64a, u32a, u32b, stdf]
		{
			stdf();
		};

		char buf[256];
		char buf2[256];
		memset(buf, 0, sizeof(buf));
		memset(buf2, 0, sizeof(buf2));

		atma::function<void()> f = std::move(L);

		using FN = atma::basic_relative_function_t<16, void()>;

		FN::make_contiguous(buf, std::move(f));

		memcpy(buf2, buf, sizeof(buf));

		auto g = (FN*)buf2;
		(*g)();

		CHECK(resulty == 4);
	}

}

SCENARIO("things")
{
	GIVEN("a directly-constructed external function too large for SFO")
	{
		char buf[128]{};
		mathing_t m;
		atma::basic_external_function_t<16, int(int)> f{atma::bind(&mathing_t::halve, &m, arg1), buf};
		auto r = f(8);
		CHECK(r == 4);
		
		atma::basic_generic_function_t<16, atma::functor_storage_t::heap, int(int)> f2{f};
		auto h = f2(12);
		CHECK(h == 6);

		using rfn = atma::basic_relative_function_t<16, int(int)>;
		char rbuf[128]{};
		new (rbuf) rfn{f, rbuf + sizeof(rfn)};
		rfn* rf = (rfn*)rbuf;
		auto rfr = (*rf)(18);
		CHECK(rfr == 9);

		char rbuf2[128];
		memcpy(rbuf2, rbuf, 128);
		rfn* fn2 = (rfn*)rbuf2;
		auto xzrfr = (*fn2)(20);
		CHECK(xzrfr == 10);

		atma::function<int(int)> lulzf = *fn2;
		auto lr = lulzf(26);
		CHECK(lr == 13);
		
		//f = *rf;
		CHECK(f(88) == 44);
	}
}
