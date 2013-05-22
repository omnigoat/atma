//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_HPP
#define ATMA_ASSERT_HPP
//=====================================================================
#include <iostream>

#include <atma/assert/config.hpp>
#include <atma/assert/switch.hpp>

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/expand.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
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
		inline handler_t& get_handler()
		{
			static atma::assert::handler_t our_handler = &hard_break_handler;
			return our_handler;
		}
	}
	
	// setting and getting the current handler
	inline void set_handler(handler_t handler)
	{
		detail::get_handler() = handler;
	}
	
	
	
	// a handful of pre-determined handlers for your convenience. the default is hard_break_handler
	inline bool hard_break_handler(const char* msg, const char* filename, size_t line)
	{
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		return true;
	}
	
	inline bool exit_failure_handler(const char* msg, const char* filename, size_t line)
	{
		std::cerr << filename << "(" << line << "): " << msg << std::endl;
		exit(EXIT_FAILURE);
		return true;
	}

	// what's called when we assert
	inline bool atma_assert(const char* msg, const char* filename, size_t line)
	{
		return (*detail::get_handler())(msg, filename, line);
	}
	
	
//=====================================================================
} // namespace assert
} // namespace atma
//=====================================================================
#define ATMA_ENSURE(x) \
	do { \
		if ( !(x) ) \
			{ __asm int 3 } \
	} while (0)

#define ATMA_ENSURE_IS(r, x) \
	do { \
		if ((r) != (x)) \
			{ __asm int 3 } \
	} while (0)

//=====================================================================
#endif // inclusion guard
//=====================================================================
