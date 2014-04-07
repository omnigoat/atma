#pragma once

#include <atma/assert/config.hpp>
#include <atma/assert/handling.hpp>

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
//=====================================================================
#ifdef ATMA_ENABLE_ASSERTS
//=====================================================================

	//=====================================================================
	// ATMA_ASSERT_MSG(x, msg)
	// -------------------------
	//   standard assert macro that takes a boolean expression @x and a
	//   string @msg and generates an assertion if !x, displaying the message.
	//=====================================================================
	#define ATMA_ASSERT_MSG(x, msg) \
		for (;;) { \
			if ( !(x) && ::atma::assert::detail::handle(msg, __FILE__, __LINE__) ) \
				{ ATMA_DEBUGBREAK(); } \
			break; \
		}


	//=====================================================================
	// ATMA_ASSERT(x)
	// ----------------
	//   Passthrough to ATMA_ASSERT_MSG that stringifies @x
	//=====================================================================
	#define ATMA_ASSERT(x) \
		ATMA_ASSERT_MSG(x, #x)


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
	#define ATMA_ASSERT_ONE_OF(...) \
		ATMA_ASSERT(( \
			ATMA_ASSERT_ONE_OF2( \
				BOOST_PP_SEQ_HEAD(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)), \
				BOOST_PP_SEQ_POP_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) )))
	
	#define ATMA_ASSERT_ONE_OF2(x, args) \
		BOOST_PP_SEQ_FOR_EACH_I(ATMA_ASSERT_ONE_OF3, x, args)

	#define ATMA_ASSERT_ONE_OF3(r, x, i, elem) \
		BOOST_PP_IF(i, ||,) (x) == (elem)

	

	
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
		

//=====================================================================
#else // ATMA_ENABLE_ASSERTS
//=====================================================================
	#define ATMA_ASSERT(...)
	#define ATMA_ASSERT_MSG(...)
	#define ATMA_ASSERT_ONE_OF(...)
	#define ATMA_ASSERT_SWITCH(...)


//=====================================================================
#endif // ATMA_ENABLE_ASSERTS
//=====================================================================
