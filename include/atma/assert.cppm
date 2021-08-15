module;

#include <iostream>
#include <source_location>
#include <atomic>

export module atma.assert;

//
// handler_t
// -----------
//  returns true if we should evoke the DEBUG_BREAK behaviour afterwards
//
//  handles the fallout when an assert is triggered. two pre-written handlers
//  are provided for convenience. we default to hard_break_handler, which
//  prints the minimum details
//
export namespace atma::assert
{
	using handler_t = auto(*)(const char* msg, const char* filename, size_t line) -> bool;

	// some pre-determined handlers for your convenience. the default is hard_break_handler
	inline bool hard_break_handler(const char* msg, const char* filename, size_t line)
	{
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		return true;
	}

	inline bool exit_failure_handler(const char* msg, const char* filename, size_t line)
	{
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		exit(-1);
		return false;
	}
}

namespace atma::assert::detail
{
	std::atomic<handler_t> global_handler_{&hard_break_handler};
}


//
// set/get the current handlers
// ------------------------------
//  users of this library can change the handler whenever. it's presumed
//  it'll happen once during startup if it happens at all though
//
export namespace atma::assert
{
	// setting and getting the current handler
	inline auto set_handler(handler_t const& handler) -> void
	{
		detail::global_handler_ = handler;
	}

	inline auto get_handler() -> handler_t
	{
		return detail::global_handler_.load();
	}
}

//
// trigger
// --------
//
export namespace atma::assert
{
	auto trigger(const char* msg, std::source_location const& L) -> bool
	{
		return (*detail::global_handler_)(msg, L.file_name(), L.line());
	}
}

