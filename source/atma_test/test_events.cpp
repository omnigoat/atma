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

		std::atomic_bool good = true;
		auto a = std::async([&] {

			atma::this_thread::set_debug_name("test thread");

			e.bind(f);
			while (good)
			{
				es.process_events_for_this_thread();
			}

		});

		a.wait_for(std::chrono::milliseconds(1000));

		e.bind(atma::detail::default_event_system, atma::detail::default_event_binder, std::this_thread::get_id(), f);
		e.raise(7);


#if 0
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
#endif
	}
}
