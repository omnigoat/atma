//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_ASSERT_SWITCH_HPP
#define ATMA_ASSERT_SWITCH_HPP
//=====================================================================
#include <atma/assert/config.hpp>
//=====================================================================
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
//=====================================================================
#if defined(ATMA_ENABLE_ASSERTS)

	//=====================================================================
	// ATMA_ASSERT_MSG
	//=====================================================================
	#define ATMA_ASSERT_MSG(x, msg) \
		do { \
			if ( !(x) && ::atma::assert::atma_assert(msg, __FILE__, __LINE__) ) \
				{ __asm int 3 } \
		} while (false)


	//=====================================================================
	// ATMA_ASSERT
	//=====================================================================
	#define ATMA_ASSERT(x) \
		ATMA_ASSERT_MSG(x, #x)


	//=====================================================================
	// ATMA_ASSERT_ONE_OF
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
	// ATMA_ASSERT_SWITCH
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
		

#else
	#define ATMA_ASSERT(...)
	#define ATMA_ASSERT_MSG(...)
	#define ATMA_ASSERT_SWITCH(...)
#endif


//=====================================================================
#endif // inclusion guard
//=====================================================================
