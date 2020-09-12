#include <atma/unit_test.hpp>

#include <atma/event.hpp>
#include <atma/lockfree_list.hpp>
#include <atma/logging.hpp>

//#include <rose/runtime.hpp>

//#include "../../shiny/include/lion/console_log_handler.hpp"


#include <future>

using namespace std::chrono_literals;

#define TEST_LOG(...) \
	::atma::send_log(::atma::default_logging_runtime(), \
		atma::log_level_t::info, nullptr, __FILE__, __LINE__, __VA_ARGS__, "\n")


#if 1
SCENARIO_OF("events", "events can be constructed")
{
	//rose::runtime_t RR;
	//rose::setup_default_logging_to_console(RR);

	std::atomic_int fin{};
	auto f = [&](int x)
	{
		//TEST_LOG("thread: ", std::this_thread::get_id(), ", x: ", x);
		if (x == 37)
			++fin;
	};

	GIVEN("a default-constructed event")
	{
		atma::event_system_t es;
		atma::event_t<int> e;

		std::atomic_int settled = 0;
		std::atomic_bool good = true;

		{
			atma::event_binder_t eb;
			e.bind(es, eb, std::this_thread::get_id(), f);
			e.raise(8);
		}
		
		e.raise(8);

		auto thread_fn = [&] {

			atma::event_binder_t eb;
			atma::this_thread::set_debug_name("test thread");
			e.bind(es, eb, std::this_thread::get_id(), f);
			
			++settled;

			while (good)
			{
				es.process_events_for_this_thread();
			}

		};

		auto a = std::thread{thread_fn};
		auto b = std::thread{thread_fn};

		while (settled != 2);

		//e.bind(atma::detail::default_event_system, atma::detail::default_event_binder, std::this_thread::get_id(), f);
		//e.bind(atma::detail::default_event_system, atma::detail::default_event_binder, b.get_id(), f);
		//e.bind(atma::detail::default_event_system, atma::detail::default_event_binder, a.get_id(), f);

		e.raise(7);
		e.raise(17);
		e.raise(27);
		e.raise(37);

		while (fin != 2);

		good = false;

		a.join();
		b.join();
	}
}
#endif
