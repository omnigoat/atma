#include <atma/unit_test.hpp>

#include <atma/event.hpp>

SCENARIO("events can be constructed", "[event]")
{
	auto f = [](int x) { std::cout << "x: " << x << std::endl; };

	GIVEN("a default-constructed event")
	{
		atma::event_system_t es;
		atma::event_t<int> e;
		atma::event_binder_t b;

		e += b + f;

		e.raise(7);
	}
}
