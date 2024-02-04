#include <atma/unit_test.hpp>
#include <atma/assert.hpp>

#include <array>
#include <utility>
#include <iostream>
#include <concepts>

import atma.memory;
import atma.intrusive_ptr;

#if 0
class A {};
class B: public A {};
class C: public A {};
class D {};

template <std::derived_from<A> T>
constexpr bool good() { return true; }
static_assert(good<D>());
#endif


struct dragon_t : atma::ref_counted
{};

using dragon_ptr = atma::intrusive_ptr<dragon_t>;

struct dragon_deleter
{
	void operator()(dragon_t* blam)
	{
		delete blam;
	}
};

SCENARIO("intrusive_ptr is created")
{
	WHEN("an intrusive_ptr<dragon_t> is default-constructed")
	{
		dragon_ptr x;
		
		THEN("it is considered null")
		{
			CHECK(x.get() == nullptr);
			CHECK(x == dragon_ptr::null);
		}
	}

	atma::intrusive_ptr<dragon_t,
		atma::use_deleter<dragon_deleter>> yay;
}

