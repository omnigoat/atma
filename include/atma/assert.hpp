//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_HPP
#define ATMA_ASSERT_HPP
//=====================================================================
#include <iostream>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/expand.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>

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
		do { \
			if ( !(x) && ::atma::assert::atma_assert(msg, __FILE__, __LINE__) ) \
				{ __asm int 3 } \
		} while (false)


	#define ATMA_ASSERT(x) ATMA_ASSERT_MSG(x, #x)

	#define ATMA_ASSERT_ONE_OF3(r, x, i, elem) \
		BOOST_PP_IF(i, ||,) (x) == (elem)

	#define ATMA_ASSERT_ONE_OF2(x, args) \
		BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_ONE_OF3, x, args)

	#define ATMA_ASSERT_ONE_OF(...) \
		ATMA_ASSERT(( \
			ATMA_ASSERT_ONE_OF2( \
				BOOST_PP_SEQ_HEAD(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)), \
				BOOST_PP_SEQ_POP_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) )))



	// wraps (a, d)(b)(c) with parenthesis ->   ((a, d))((b))((c))
	#define ADD_PAREN_1(...) ((__VA_ARGS__)) ADD_PAREN_2 
	#define ADD_PAREN_2(...) ((__VA_ARGS__)) ADD_PAREN_1 
	#define ADD_PAREN_1_END 
	#define ADD_PAREN_2_END 
	#define PARENTHIFY(s,d,e) (e)
	#define SEQUIFY(kindof_seq) \
		BOOST_PP_CAT(ADD_PAREN_1 kindof_seq,_END)
		
			

	#define ATMA_ASSERT_SWITCH_CASE_LABEL_M(r,d,i,elem) \
		case BOOST_PP_CAT(elem, :)

	#define ATMA_ASSERT_SWITCH_CASE_LABEL(r,seq) \
		BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_SWITCH_CASE_LABEL_M, _, seq)

	#define ATMA_ASSERT_SWITCH_CASE(r,_,elem) \
		ATMA_ASSERT_SWITCH_CASE_LABEL(r, BOOST_PP_SEQ_POP_BACK(BOOST_PP_TUPLE_TO_SEQ(elem))) \
		{ \
			BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(BOOST_PP_TUPLE_TO_SEQ(elem))); \
			break; \
		}

	#define ATMA_ASSERT_SWITCH(x, seq) \
		do { \
			switch (x) { \
				BOOST_PP_SEQ_FOR_EACH(ATMA_ASSERT_SWITCH_CASE,, SEQUIFY(seq)) \
			} \
		} while(0)



#else
	#define ATMA_ASSERT(x)
	#define ATMA_ASSERT_MSG(x,m)

#endif

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
