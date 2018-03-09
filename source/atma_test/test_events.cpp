#include <atma/unit_test.hpp>

#include <atma/event.hpp>

#include <future>

using namespace std::chrono_literals;

SCENARIO("events can be constructed", "[event]")
{
	auto f = [](int x) { std::cout << "thread: " << std::this_thread::get_id() << ", x: " << x << std::endl; };

	GIVEN("a default-constructed event")
	{
		atma::event_system_t es;
		atma::event_t<int> e;
		atma::event_binder_t b;

		e += b + f;

		e.raise(7);

		std::atomic_bool good = true;
		auto a = std::async([&] {

			e.bind(f);
			while (good)
			{
				es.process_events_for_this_thread();
			}

		});

		a.wait_for(1s);
		e.raise(9);
		e.raise(11);
		e.raise(2);
		a.wait_for(1s);
		good = false;
	}
}
