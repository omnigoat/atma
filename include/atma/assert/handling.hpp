//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_HANDLING_HPP
#define ATMA_ASSERT_HANDLING_HPP
//=====================================================================
#include <iostream>
//=====================================================================
namespace atma {
namespace assert {
//=====================================================================

	
	// typedefs/forward declares
	typedef bool (*handler_t)(const char* msg, const char* filename, size_t line);
	bool hard_break_handler(const char* msg, const char* filename, size_t line);
	bool exit_failure_handler(const char* msg, const char* filename, size_t line);

	namespace detail
	{
		inline auto get_handler() -> handler_t& {
			static atma::assert::handler_t our_handler = &hard_break_handler;
			return our_handler;
		}

		// what's called when we assert
		inline auto handle(const char* msg, const char* filename, size_t line) -> bool {
			return (*get_handler())(msg, filename, line);
		}
	}
	
	
	// setting and getting the current handler
	inline auto set_handler(handler_t const& handler) -> void {
		detail::get_handler() = handler;
	}

	inline auto get_handler() -> handler_t const& {
		return detail::get_handler();
	}

	



	// some pre-determined handlers for your convenience. the default is hard_break_handler
	inline bool hard_break_handler(const char* msg, const char* filename, size_t line) {
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		return true;
	}
	
	inline bool exit_failure_handler(const char* msg, const char* filename, size_t line) {
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		exit(EXIT_FAILURE);
		return false;
	}

	
	
	
//=====================================================================
} // namespace assert
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
