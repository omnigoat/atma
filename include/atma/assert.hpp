//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_HPP
#define ATMA_ASSERT_HPP
//=====================================================================
#include <iostream>

//=====================================================================
// setting up whether we assert or not
//=====================================================================
#if _DEBUG
#	define ATMA_ENABLE_ASSERTS
#else
#	define ATMA_ENABLE_ASSERTS
#endif

#ifdef ATMA_NO_ASSERT
#	undef ATMA_ENABLE_ASSERTS
#endif


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
#if defined(ATMA_ENABLE_ASSERTS)
	#define ATMA_ASSERT_MSG(x, msg) \
		([](int line){ \
			if ( !(x) && ::atma::assert::atma_assert(msg, __FILE__, line) ) \
				{ __asm int 3 }\
		})(__LINE__)


	#define ATMA_ASSERT(x) ATMA_ASSERT_MSG(x, #x)
		
#else
	#define ATMA_ASSERT(x) ([]{})()
	#define ATMA_ASSERT_MSG(x,m) ([]{})()

#endif

//=====================================================================
#endif // inclusion guard
//=====================================================================
