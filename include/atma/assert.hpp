#pragma once

#include <atma/config/platform.hpp>

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>

#include <iostream>

//=====================================================================
//
//    CONFIG
//
//=====================================================================
#ifdef ATMA_PLATFORM_WINDOWS
#	define ATMA_DEBUGBREAK() __debugbreak()
#else
#	define ATMA_DEBUGBREAK()
#endif

#if !defined(ATMA_ENABLE_ASSERTS)
#  if _DEBUG
#    define ATMA_ENABLE_ASSERTS true
#  else
#    define ATMA_ENABLE_ASSERTS false
#  endif
#endif



#define ATMA_UNUSED_II(s, d, e) (void)e

#define ATMA_UNUSED(...) \
	BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(ATMA_UNUSED_II, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))



//=====================================================================
//
//    HANDLING
//
//=====================================================================
namespace atma { namespace assert {


	using handler_t = auto(*)(const char* msg, const char* filename, size_t line) -> bool;

	auto hard_break_handler(const char* msg, const char* filename, size_t line) -> bool;
	auto exit_failure_handler(const char* msg, const char* filename, size_t line) -> bool;

	namespace detail
	{
		inline auto get_handler() -> handler_t& {
			static atma::assert::handler_t _ = &hard_break_handler;
			return _;
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
}}




//=====================================================================
// ATMA_HALT()
// -------------------------
//   halts execution via the debug-break handler. never turned off.
//=====================================================================
#define ATMA_HALT(msg) \
	do { \
		ATMA_DEBUGBREAK(); \
		break; \
	} while(0)


//=====================================================================
// ATMA_ENSURE
// -------------
//   ensures an expression equates to true. never turned off.
//=====================================================================
#define ATMA_ENSURE_III(x, msg) \
	do { \
		if ( !(x) && ::atma::assert::detail::handle(msg, __FILE__, __LINE__) ) \
			{ ATMA_DEBUGBREAK(); } \
		break; \
	} while(0)

#define ATMA_ENSURE_II_1(x) ATMA_ENSURE_III(x, #x)
#define ATMA_ENSURE_II_1_(args) ATMA_ENSURE_II_1 args
#define ATMA_ENSURE_II_2(x, m) ATMA_ENSURE_III(x, m)
#define ATMA_ENSURE_II_2_(args) ATMA_ENSURE_II_2 args

#define ATMA_ENSURE(...) \
	BOOST_PP_CAT(ATMA_ENSURE_II_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__))_((__VA_ARGS__))


//=====================================================================
// ATMA_ENSURE_IS
// ----------------
//   ensures that the expression @x evaluates to @r
//=====================================================================
#define ATMA_ENSURE_IS(r, x) \
	do { \
		auto _xr = (x); \
		ATMA_ENSURE((r) == _xr); \
	} while (0)


//=====================================================================
// ATMA_ASSERT(...)
// ------------------
//   two overloads
//     - first takes the simple boolean expression, strigifies it
//     - second takes the expression + info string
//=====================================================================
#if ATMA_ENABLE_ASSERTS
	#define ATMA_ASSERT(...) ATMA_ENSURE(__VA_ARGS__)
#else
	#define ATMA_ASSERT(...)
#endif


//=====================================================================
// ATMA_ASSERT_ONE_OF(x, ...)
// ----------------------------
//   @x is compared against all subsequent arguments with the equality
//   operator, and that boolean expression is given to ATMA_ASSERT. the
//   expression generated (and given to ATMA_ASSERT) is as follows:
//
//   ATMA_ASSERT_ONE_OF(my_thing, option1, option2, option3)
//
//   my_thing == option1 || my_thing == option2 || my_thing == option3.
//
//=====================================================================
#if ATMA_ENABLE_ASSERTS
	#define ATMA_ASSERT_ONE_OF(...) \
		ATMA_ASSERT(( \
			ATMA_ASSERT_ONE_OF2( \
				BOOST_PP_SEQ_HEAD(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)), \
				BOOST_PP_SEQ_POP_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) )))
	
	#define ATMA_ASSERT_ONE_OF2(x, args) \
		BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_ONE_OF3, x, args)

	#define ATMA_ASSERT_ONE_OF3(r, x, i, elem) \
		BOOST_PP_IF(i, ||,) (x) == (elem)
#else
	#define ATMA_ASSERT_ONE_OF(...)
#endif


//=====================================================================
// ATMA_ASSERT_SWITCH(x, seq)
// ----------------------------
//   takes a value @x and a sequence @seq. @seq is a sequence of
//   tuples, of minimum size 2, where the last tuple element is the
//   "action" to perform, and the first n-1 elements are values to
//   compare @x against. the first n-1 elements are case labels:
//
//   ATMA_ASSERT_SWITCH(my_thing, (2, thing2)(4, 5, thing45));
//
//   switch(my_thing) {
//     case 2: {
//       thing2;
//       break;
//     }
//     case 4:
//     case 5: {
//       thing45;
//       break;
//     }
//   }
//
//=====================================================================
#if false
	#define ATMA_ASSERT_SWITCH(x, seq) \
		do { \
			switch (x) { \
				BOOST_PP_SEQ_FOR_EACH(ATMA_ASSERT_SWITCH_CASE,, SEQUIFY(seq)) \
			} \
		} while(0)

	#define ATMA_ASSERT_SWITCH_CASE(r,_,elem) \
		ATMA_ASSERT_SWITCH_CASE_LABEL(r, BOOST_PP_SEQ_POP_BACK(BOOST_PP_TUPLE_TO_SEQ(elem))) \
		{ \
			BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REVERSE(BOOST_PP_TUPLE_TO_SEQ(elem))); \
			break; \
		}

	#define ATMA_ASSERT_SWITCH_CASE_LABEL(r,seq) \
		BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_SWITCH_CASE_LABEL_M, _, seq)

	#define ATMA_ASSERT_SWITCH_CASE_LABEL_M(r,d,i,elem) \
		case BOOST_PP_CAT(elem, :)


	// wraps (a, d)(b)(c) with parenthesis ->   ((a, d))((b))((c))
	#define ADD_PAREN_1(...) ((__VA_ARGS__)) ADD_PAREN_2 
	#define ADD_PAREN_2(...) ((__VA_ARGS__)) ADD_PAREN_1 
	#define ADD_PAREN_1_END 
	#define ADD_PAREN_2_END 
	#define PARENTHIFY(s,d,e) (e)
	#define SEQUIFY(kindof_seq) \
		BOOST_PP_CAT(ADD_PAREN_1 kindof_seq,_END)
#endif
