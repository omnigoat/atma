#ifndef ATMA_FUNCTOR_HPP
#define ATMA_FUNCTOR_HPP
//=====================================================================
#include <boost/preprocessor.hpp>
//=====================================================================


//=====================================================================
//
//  functors!
//
//=====================================================================
// template parameters: typename T1, typename T2...
// template arguments:  T1, T2...
// parameters: T1& arg1, T2& arg2...
// arguments: arg1, arg2...
// references T1& arg2; T2& arg2; ...
// initializer_list: arg1(arg1), arg2(arg2)...

#define ATMA_functor_template_parameter(z, n, t) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) typename t##n
#define ATMA_functor_template_parameters(argcount, auxcount) \
		BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_template_parameter, T) \
		BOOST_PP_COMMA_IF( BOOST_PP_AND(argcount, auxcount) ) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(auxcount), ATMA_functor_template_parameter, A)

#define ATMA_functor_template_argument(z, n, t) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) t##n

#define ATMA_functor_template_arguments(argcount, auxcount) \
		BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_template_argument, T) \
		BOOST_PP_COMMA_IF( BOOST_PP_AND(argcount, auxcount) ) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(auxcount), ATMA_functor_template_argument, A)



#define ATMA_functor_parameter(z, n, t) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) T##n##& arg##n
#define ATMA_functor_parameters(argcount) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_parameter, ~)

#define ATMA_functor_argument(z, n, t) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) arg##n
#define ATMA_functor_arguments(argcount) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_argument, ~)

#define ATMA_functor_reference(z, n, t) T##n##& arg##n;
#define ATMA_functor_references(argcount) \
	BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_reference, ~)

#define ATMA_functor_initializer_list_item(z, n, t) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) arg##n##(arg##n##)
#define ATMA_functor_initializer_list(argcount) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(argcount), ATMA_functor_initializer_list_item, ~)

//#define G(a, b) a ## BOOST_PP_COMMA_IF( BOOST_PP_ADD(a, b) ) ## b
#define ATMA_functor_template_declaration(argcount, auxcount) \
	BOOST_PP_EXPR_IF( \
		BOOST_PP_ADD(argcount, auxcount), \
		template< ATMA_functor_template_parameters(argcount, auxcount) > \
	)
//
#define ATMA_functor_members(argcount) \
		ATMA_functor_references(argcount)

#define ATMA_functor_constructor(name, argcount) \
	BOOST_PP_EXPR_IF( \
		argcount, \
		name##_a( ATMA_functor_parameters(argcount) ) : ATMA_functor_initializer_list(argcount) {} \
	)

#define ATMA_functor_operator(return_type, parameters) \
	return_type operator()( BOOST_PP_SEQ_ENUM(parameters) );


#define ATMA_FUNCTOR_TYPE(name, argcount, auxcount) \
	BOOST_PP_CAT( \
		name##_a, \
		BOOST_PP_EXPR_IF( \
			BOOST_PP_ADD(argcount, auxcount), \
			< ATMA_functor_template_arguments(argcount, auxcount) > \
		) \
	)

#define ATMA_functor_unparen2(a) a
#define ATMA_functor_unparen(a) ATMA_functor_unparen2##a

#define ATMA_FUNCTOR(name, argcount, auxcount, return_type, parameters) \
	ATMA_functor_template_declaration(argcount, auxcount) \
	struct name##_a \
	{ \
		ATMA_functor_members(argcount) \
		ATMA_functor_constructor(name, argcount) \
		ATMA_functor_operator(return_type, parameters) \
	}; \
	ATMA_functor_template_declaration(argcount, auxcount) \
	ATMA_FUNCTOR_TYPE(name, argcount, auxcount) \
	name( ATMA_functor_parameters(argcount) ) \
	{ \
		return ATMA_FUNCTOR_TYPE(name, argcount, auxcount)( ATMA_functor_arguments(argcount) ); \
	} \
	 \
	ATMA_functor_template_declaration(argcount, auxcount) \
	return_type ATMA_FUNCTOR_TYPE(name, argcount, auxcount)##::operator ()( BOOST_PP_SEQ_ENUM(parameters) )


//#undef ATMA_functor_argument
//#undef ATMA_functor_arguments
//#undef ATMA_functor_constructor
//#undef ATMA_functor_initializer_list
//#undef ATMA_functor_initializer_list_item
//#undef ATMA_functor_members
//#undef ATMA_functor_operator
//#undef ATMA_functor_parameter
//#undef ATMA_functor_parameters
//#undef ATMA_functor_reference
//#undef ATMA_functor_references
//#undef ATMA_functor_template_declaration
//#undef ATMA_functor_template_argument
//#undef ATMA_functor_template_arguments
//#undef ATMA_functor_template_parameter
//#undef ATMA_functor_template_parameters


//=====================================================================
#endif // inclusion guard
//=====================================================================



