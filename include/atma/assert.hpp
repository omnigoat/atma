#pragma once

#include <atma/config/platform.hpp>

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>

#include <iostream>
#include <source_location>

#ifdef ATMA_PLATFORM_WINDOWS
#	define ATMA_DEBUGBREAK() __debugbreak()
#else
#	define ATMA_DEBUGBREAK()
#endif


#if !defined(ATMA_ENABLE_ASSERTS)
#  if NDEBUG
#    define ATMA_ENABLE_ASSERTS false
#  else
#    define ATMA_ENABLE_ASSERTS true
#  endif
#endif


//
// ATMA_HALT()
// -------------------------
//   halts execution via the debug-break handler. never turned off.
//
#define ATMA_HALT(msg) \
	do { \
		::atma::assert::trigger(msg); \
		ATMA_DEBUGBREAK(); \
		break; \
	} while(0)


//
// ATMA_ENSURE(...)
// ------------------
//   two overloads
//     - first takes the simple boolean expression, strigifies it
//     - second takes the expression + info string
//
#define ATMA_ENSURE_III(x, msg) \
	do { \
		if ( !(x) && ::atma::assert::trigger(msg) ) \
			{ ATMA_DEBUGBREAK(); } \
		break; \
	} while(0)

#define ATMA_ENSURE_II_1(x) ATMA_ENSURE_III(x, #x)
#define ATMA_ENSURE_II_1_(args) ATMA_ENSURE_II_1 args
#define ATMA_ENSURE_II_2(x, m) ATMA_ENSURE_III(x, m)
#define ATMA_ENSURE_II_2_(args) ATMA_ENSURE_II_2 args

#define ATMA_ENSURE(...) \
	BOOST_PP_CAT(ATMA_ENSURE_II_, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__))_((__VA_ARGS__))


//
// ATMA_ASSERT(...)
// ------------------
//  essentially the same as ATMA_ENSURE, except it is turned off
//  by ATMA_ENABLE_ASSERTS, where ATMA_ENSURE is never changed
//
#if ATMA_ENABLE_ASSERTS
#  define ATMA_ASSERT(...) ATMA_ENSURE(__VA_ARGS__)
#else
#  define ATMA_ASSERT(...)
#endif


//
// ATMA_ASSERT_ONE_OF(x, ...)
// ----------------------------
//  @x is compared against all subsequent arguments with the equality
//  operator, and that boolean expression is given to ATMA_ASSERT. the
//  expression generated (and given to ATMA_ASSERT) is as follows:
//
//  ATMA_ASSERT_ONE_OF(my_thing, option1, option2, option3)
//
//  my_thing == option1 || my_thing == option2 || my_thing == option3.
//
//
#if ATMA_ENABLE_ASSERTS
#  define ATMA_ASSERT_ONE_OF3(r, x, i, elem) BOOST_PP_IF(i, ||,) (x) == (elem)
#  define ATMA_ASSERT_ONE_OF2(x, args) BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_ONE_OF3, x, args)
#  define ATMA_ASSERT_ONE_OF(...) \
	ATMA_ASSERT(( \
		ATMA_ASSERT_ONE_OF2( \
			BOOST_PP_SEQ_HEAD(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)), \
			BOOST_PP_SEQ_POP_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) )))
#else
#  define ATMA_ASSERT_ONE_OF(...)
#endif




//
// declaration of atma::assert::trigger
//
namespace atma::assert
{
	auto trigger(const char* msg, std::source_location const& = std::source_location::current()) -> bool;
}