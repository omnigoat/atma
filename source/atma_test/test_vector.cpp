
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

#define SELECT_PLATFORM_case_PLATFORM_WIN64(...) 0
#define SELECT_PLATFORM_case_PLATFORM_LINUX64(...) 0
#define SELECT_PLATFORM_case_PLATFORM_ANDROID(...) 1
#define SELECT_PLATFORM_case_PLATFORM_IPHONE(...) 0
#define SELECT_PLATFORM_case_PLATFORM_SWITCH(...) 0
#define SELECT_PLATFORM_case_failure(...) 1

#define ID_PLATFORM_case_PLATFORM_WIN64 1
#define ID_PLATFORM_case_PLATFORM_LINUX64 2
#define ID_PLATFORM_case_PLATFORM_ANDROID 5
#define ID_PLATFORM_case_PLATFORM_IPHONE 6
#define ID_PLATFORM_case_PLATFORM_SWITCH 7


#define ES3(r, s, x) BOOST_PP_ADD(s, x)
#define ES2(r, s, t) \
	(BOOST_PP_EQUAL( \
		BOOST_PP_CAT(ID_PLATFORM_case_, BOOST_PP_TUPLE_ELEM(0, BOOST_PP_TUPLE_ELEM(0, t))), \
		BOOST_PP_CAT(ID_PLATFORM_case_, BOOST_PP_TUPLE_ELEM(1, t))))

	//BOOST_PP_EQUAL(member, BOOST_PP_TUPLE_ELEM(0, candidate))
	
//#define ESS(r, member, candidates) BOOST_PP_SEQ_FOR_EACH_R(r, ES2, member, (one)(two)(three))
//#define ESS2(r, candidates, member) ESS(r, candidates, member)
#define ES(r, product) (BOOST_PP_SEQ_TO_TUPLE(product))

	//candidates_ >> candidates ||| member_ >> member
	//(BOOST_PP_SEQ_FOLD_LEFT(ES3, 0, BOOST_PP_SEQ_FOR_EACH(ES2, member, candidates)))

#define failure(mname, x) (failure, TW(__LINE__, mname - missing entry for exhaustive list))
#define unwrap(mname, x) x

#define PLATFORM_EXHAUSTIVE_SEARCH(mname, members, candidates) \
	BOOST_PP_IF(BOOST_PP_EQUAL( \
		BOOST_PP_SEQ_SIZE(members), \
		BOOST_PP_SEQ_FOLD_LEFT(ES3, 0,  \
			BOOST_PP_SEQ_FOR_EACH(ES2, 0, \
				BOOST_PP_SEQ_FOR_EACH_PRODUCT(ES, (candidates)(members))))), \
		unwrap, \
		failure) (mname, BOOST_PP_SEQ_ENUM(candidates))

#define LOW_TIER_PLATFORMS_LIST \
	(PLATFORM_ANDROID)\
	(PLATFORM_IPHONE)\
	(PLATFORM_SWITCH)

#define LOW_TIER_PLATFORMS_EXHAUSTIVE(...) \
	PLATFORM_EXHAUSTIVE_SEARCH( \
		LOW_TIER_PLATFORMS, \
		(PLATFORM_ANDROID)(PLATFORM_IPHONE)(PLATFORM_SWITCH), \
		BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))



//LOW_TIER_PLATFORMS_EXHAUSTIVE(
//	(PLATFORM_ANDROID, 3),
//	(PLATFORM_SWITCH, 8),
//	(PLATFORM_IPHONE, 77))

#define SELECT_PLATFORM_subcategory_PHYSICS_TRICKY_case_PLATFORM_WIN64(s, x) 0

#define TW2(msg) _Pragma(#msg)
#define TW1(msg) TW2(message(msg))
//#define TW00(msg) TW1(message(#msg))
#define TW00(file, line, msg) TW1(file "(" #line "): error a la Frostbite: " #msg)
#define TW0(file, line, msg) TW00(file, line, msg)
#define TW(line, msg) TW0(__FILE__, line, msg)
//#define BOOST_PP_IIF_BOOST_PP_BOOL__woah_unspecified_platform(...) TW(_Pragma("message(\"error : " #__FILE__ "you've missed a platform for this exhaustive list\")")

#define CALC2_HAVE_RESULT_3(state, x, name) SELECT_PLATFORM_case_##name(state, x)
#define CALC2_HAVE_RESULT_2(state, x, name) CALC2_HAVE_RESULT_3(state, x, name)
#define CALC2_HAVE_RESULT(state, x, name) CALC2_HAVE_RESULT_2(state, x, name)


#define USOP(d, state, x) \
	BOOST_PP_OR(state, CALC2_HAVE_RESULT(state, x, BOOST_PP_TUPLE_ELEM(0, x)))


// this is highly dependant on the implementation of boost
#define BOOST_PP_IIF_BOOST_PP_BOOL__woah_unspecified_platform(...) \
	TW(__LINE__, "you've missed a platform for this exhaustive list")

#define SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2_1() 1
#define SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2_0() _woah_unspecified_platform

#define SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2(state, x, n) \
	BOOST_PP_IF(GET_EXH(state), \
		BOOST_PP_CAT(SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2_, n)(), \
		SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2_1())

#define SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED(state, x) \
	SELECT_PLATFORM_case_PLATFORM_UNSPECIFIED_2(state, x, \
		BOOST_PP_SEQ_FOLD_LEFT(USOP, 0, \
			BOOST_PP_SEQ_REMOVE(BOOST_PP_TUPLE_ELEM(3, state), \
			BOOST_PP_TUPLE_ELEM(2, x))))
	

#define GET_RESULT(state) BOOST_PP_TUPLE_ELEM(0, state)
#define GET_PLAT_FOUND(state) BOOST_PP_TUPLE_ELEM(1, state)
#define GET_EXH(state) BOOST_PP_TUPLE_ELEM(2, state)
#define GET_PLAT_LIST(state) BOOST_PP_TUPLE_ELEM(3, state)

#define CALC_HAVE_RESULT_3(state, x, name) SELECT_PLATFORM_case_##name(state, x)
#define CALC_HAVE_RESULT_2(state, x, name) CALC_HAVE_RESULT_3(state, x, name)
#define CALC_HAVE_RESULT(state, x, name) \
	BOOST_PP_IF(BOOST_PP_NOT(BOOST_PP_TUPLE_ELEM(1, state)), \
		CALC_HAVE_RESULT_2(state, x, name), \
		BOOST_PP_TUPLE_ELEM(1, state))

#define CALC_RESULT(state, x) \
	BOOST_PP_IF(BOOST_PP_NOT(BOOST_PP_TUPLE_ELEM(1, state)),\
		BOOST_PP_IF(BOOST_PP_CAT(SELECT_PLATFORM_case_, BOOST_PP_TUPLE_ELEM(0, x))(state, x), \
			BOOST_PP_TUPLE_ELEM(1, x),\
			BOOST_PP_TUPLE_ELEM(0, state)),\
		BOOST_PP_TUPLE_ELEM(0, state))

#define PRED(r, state) \
	BOOST_PP_EQUAL(GET_PLAT_FOUND(state), 0)

#define OP(d, state, x) \
	( \
		CALC_RESULT(state, x), \
		CALC_HAVE_RESULT(state, x, BOOST_PP_TUPLE_ELEM(0, x)), \
		GET_EXH(state), \
		GET_PLAT_LIST(state) \
	)

#define M(r, state) \
	state

#define PERFORM_NONEXHAUSTIVE 0
#define PERFORM_EXHAUSTIVE 1

// (result, have-result, exhaustive, seq)
#define BLAMMO3(exh, seq) BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_FOLD_LEFT(OP, (~, 0, exh, seq), seq))
#define TR(s, d, i, elem) (BOOST_PP_TUPLE_PUSH_BACK(elem, i))
#define BLAMMO2(exh, seq) BLAMMO3(exh, BOOST_PP_SEQ_FOR_EACH_I(TR, ~, seq))
#define COMPILER_SWITCH(...) BLAMMO2(PERFORM_NONEXHAUSTIVE, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define COMPILER_SWITCH_EXHAUSTIVE(...) BLAMMO2(PERFORM_EXHAUSTIVE, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define HOORAY COMPILER_SWITCH( \
	(PLATFORM_WIN64, hello), \
	(PLATFORM_LINUX64, goodbye), \
	(PLATFORM_UNSPECIFIED, where_am_i))


#define HUZZAH COMPILER_SWITCH( \
	(PLATFORM_WIN64, "hello"), \
	(PLATFORM_LINUX64, "goodbye"), \
	\
	LOW_TIER_PLATFORMS_EXHAUSTIVE( \
		(PLATFORM_ANDROID, "whoops"), \
		(PLATFORM_SWITCH, "my bad"), \
		(PLATFORM_IPHONE, "everyone")), \
	\
	(PLATFORM_UNSPECIFIED, "oh no"))



#define CACHE_LINE_SIZE \
	FB_PLATFORM_SWITCH( \
		(EA_PLATFORM_WINDOWS, "hello") \
		(EA_PLATFORM_LINUX, "goodbye") \
		\
		LOW_TIER_PLATFORMS_EXHAUSTIVE( \
			(EA_PLATFORM_ANDROID, "whoops") \
			(EA_PLATFORM_SWITCH, "my bad") \
			(EA_PLATFORM_IPHONE, "everyone")) \
		\
		(EA_PLATFORM_UNSPECIFIED, "oh no"))

template <size_t N>
struct string_literal
{
	constexpr string_literal(const char(&str)[N])
	{
		std::copy_n(str, N, data);
	}

	char data[N];
};

template <string_literal>
struct print {};

int blam()
{

	//LOW_TIER_PLATFORMS_EXHAUSTIVE(\
	//	(PLATFORM_ANDROID, 3), \
	//	/*(PLATFORM_SWITCH, 8), */\
	//	(PLATFORM_IPHONE, 77));

	//constexpr char const* r = HUZZAH;

	
}



template <typename F, typename R>
struct map_range
{
	map_range(auto&& f, auto&& r)
		: f_{std::forward<decltype(f)>(f)}
		, r_{std::forward<decltype(r)>(r)}
	{}

	// apply 
	void execute(auto&& permutator, auto&& returnor)
	{
		for (auto i = std::begin(r_), ie = std::end(r_); i != ie; ++i)
		{
			auto&& x = *i;
			auto p = permutator(x);
			if (auto [r, yes] = returnor(p); yes)
				return r;
		}
	}

	void apply(auto&& g)
	{
		//for (auto&& x : r_)
		//{
		//	g(f_(r_));
		//}

		r_.apply([&](auto&& x) { return g(f_(x)); });
	}

private:
	F f_;
	R r_;
};

