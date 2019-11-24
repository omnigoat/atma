#pragma once

#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/variadic.hpp>

//
// GENERATE_COMBINATIONS_OF_TUPLES
// ---------------------------------
//  takes a list of tuple-seqs and generates a sequence of tuples that corresponds
//  to each product:
//
//    GENERATE_COMBINATIONS_OF_TUPLES((int, float)(string, char*))
//
//        gives:
//          (int, string)(int, char*)(float, string)(float, char*)
//
#define GENERATE_COMBINATIONS_OF_TUPLES_m(r, product) (BOOST_PP_SEQ_TO_TUPLE(product))
#define GENERATE_COMBINATIONS_OF_TUPLES_t(s, data, elem) BOOST_PP_TUPLE_TO_SEQ(elem)
#define GENERATE_COMBINATIONS_OF_TUPLES(tupleseqs) \
	BOOST_PP_SEQ_FOR_EACH_PRODUCT(GENERATE_COMBINATIONS_OF_TUPLES_m, \
			BOOST_PP_SEQ_TRANSFORM(GENERATE_COMBINATIONS_OF_TUPLES_t, ~, \
				BOOST_PP_VARIADIC_SEQ_TO_SEQ(tupleseqs)))

//
// FOR_EACH_COMBINATION(fn, data, tupleseq)
// -----------------------------------------------------------------
//  takes a template-typename from @type_name, and generates all possible combinations
//  from the given seq. example:
//
//    FOR_EACH_COMBINATION(fn, data, (int, float)(string, char*))
//
//  NOTE: this is needlessly complex because MSVC is non-conformant :(
//
#define FOR_EACH_COMBINATION_EXPAND(x) x
#define FOR_EACH_COMBINATION_EXPAND_F(m, ...) FOR_EACH_COMBINATION_EXPAND(m BOOST_PP_LPAREN() __VA_ARGS__ BOOST_PP_RPAREN())
#define FOR_EACH_COMBINATION_m2(f, ...) FOR_EACH_COMBINATION_EXPAND_F(f, __VA_ARGS__)
#define FOR_EACH_COMBINATION_m(r,d,i,x) FOR_EACH_COMBINATION_m2(BOOST_PP_TUPLE_ELEM(2, 0, d), i, BOOST_PP_TUPLE_ELEM(2, 1, d), BOOST_PP_TUPLE_REM_CTOR(x))
#define FOR_EACH_COMBINATION(fn, d, tupleseq) \
	BOOST_PP_SEQ_FOR_EACH_I(FOR_EACH_COMBINATION_m, (fn, d), \
		tupleseq)

//
// GENERATE_TEMPLATE_TYPE_COMBINATIONS(type_name, combination_seq)
// -----------------------------------------------------------------
//  takes a template-typename from @type_name, and generates all possible combinations
//  from the given seq. example:
//
//    GENERATE_TEMPLATE_TYPE_COMBINATIONS(dragon_t, (int, float)(string, char*))
//
//        gives:
//          (dragon_t<int, string>)(dragon_t<int, char*>)(dragon_t<float, string>)(dragon_t<float, char*>)
//
#define GENERATE_TEMPLATE_TYPE_COMBINATIONS_M(i, d, ...) ((d<__VA_ARGS__>))
#define GENERATE_TEMPLATE_TYPE_COMBINATIONS(type_name, tupleseq) \
	FOR_EACH_COMBINATION(GENERATE_TEMPLATE_TYPE_COMBINATIONS_M, type_name, \
		GENERATE_COMBINATIONS_OF_TUPLES(tupleseq))


//
// FOR_EACH_TEMPLATE_TYPE_COMBINATION(fn, type_name, combination_seq)
// -----------------------------------------------------------------
//  applies a function over every template-type-combination
//
//    #define BLAMMO(i, t1, t2) type1=t2, type2=t2
//    GENERATE_TEMPLATE_TYPE_COMBINATIONS(dragon_t, (int, float)(string, char*))
//
#define FOR_EACH_TEMPLATE_TYPE_COMBINATION_F(r,d,i,x) FOR_EACH_COMBINATION_EXPAND_F(d, i, BOOST_PP_TUPLE_REM_CTOR(x))
#define FOR_EACH_TEMPLATE_TYPE_COMBINATION(fn, tupleseq) \
	BOOST_PP_SEQ_FOR_EACH_I(FOR_EACH_TEMPLATE_TYPE_COMBINATION_F, fn, tupleseq)


