#include <atma/unit_test.hpp>

#include <atma/event.hpp>
#include <atma/lockfree_list.hpp>
#include <atma/logging.hpp>

#include <rose/runtime.hpp>

#include "../../shiny/include/lion/console_log_handler.hpp"


#include <future>

using namespace std::chrono_literals;

#define TEST_LOG(level, ...) \
	::atma::send_log(&LR, \
		level, nullptr, __FILE__, __LINE__, __VA_ARGS__, "\n")


SCENARIO("events can be constructed", "[event]")
{
	atma::logging_runtime_t LR;

	rose::runtime_t RR;
	lion::console_log_handler_t console_log{RR.get_console()};
	LR.attach_handler(&console_log);

	auto f = [&LR](int x)
	{
		TEST_LOG(atma::log_level_t::info, "thread: ", std::this_thread::get_id(), ", x: ", x);
	}; // std::cout << "thread: " << std::this_thread::get_id() << ", x: " << x << std::endl; };

	GIVEN("a default-constructed event")
	{
		atma::event_system_t es;
		atma::event_t<int> e;

		std::atomic_int settled;
		std::atomic_bool good = true;

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

		//good = false;
		a.join();
		b.join();

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
