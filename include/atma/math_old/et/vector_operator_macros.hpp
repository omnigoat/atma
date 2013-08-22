//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_VECTOR_OPERATOR_MACROS_HPP
#define ATMA_MATH_ET_VECTOR_OPERATOR_MACROS_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et { // irrelevant really, macros
//=====================================================================

	//========================================================================
	// Binary: Type op Type
	//========================================================================
	#define ATMA_MATH_ET_BINARY_TT( r_, t1_, t2_, oper_def_, oper_ )					\
	inline friend																		\
	et::expr																			\
	<																					\
		r_,																				\
		et::binary_expr																	\
		<																				\
			r_,																			\
			t1_,																		\
			t2_,																		\
			oper_																		\
		>																				\
	>																					\
	operator oper_def_ (const t1_& lhs, const t2_& rhs)									\
	{																					\
		return et::make_expr( et::make_binary_expr<r_, oper_>(lhs, rhs) );				\
	}																					
	
	
	//========================================================================
	// Binary: Expr op Type
	//========================================================================
	#define ATMA_MATH_ET_BINARY_XT( r_, t1_, t2_, oper_def_, oper_ )							\
	template <typename XT>																		\
	inline friend																				\
	et::expr																					\
	<																							\
		r_,																						\
		et::binary_expr																			\
		<																						\
			r_,																					\
			typename et::expr<t1_, XT>::expression_type,										\
			t2_,																				\
			oper_																				\
		>																						\
	>																							\
	operator oper_def_ (const et::expr<t1_, XT>& lhs, const t2_& rhs)							\
	{																							\
		return et::make_expr( et::make_binary_expr<r_, oper_>(lhs.expression(), rhs) );			\
	}																									
	
	
	//========================================================================
	// Binary: Type op Expr
	//========================================================================
	#define ATMA_MATH_ET_BINARY_TX( r_, t1_, t2_, oper_def_, oper_ )							\
	template <typename XT>																		\
	inline friend																				\
	et::expr																					\
	<																							\
		r_,																						\
		et::binary_expr																			\
		<																						\
			r_,																					\
			t1_,																				\
			typename et::expr<t2_, XT>::expression_type,										\
			oper_																				\
		>																						\
	>																							\
	operator oper_def_ (const t1_& lhs, const et::expr<t2_, XT>& rhs)							\
	{																							\
		return et::make_expr( et::make_binary_expr<r_, oper_>(lhs, rhs.expression()) );			\
	}
	
	//========================================================================
	// Binary: Expr op Expr
	//========================================================================
	#define ATMA_MATH_ET_BINARY_XX( r_, t1_, t2_, oper_def_, oper_ )							\
	template <typename XT, typename YT>															\
	inline friend																				\
	et::expr																					\
	<																							\
		r_,																						\
		et::binary_expr																			\
		<																						\
			r_,																					\
			typename et::expr<t1_, XT>::expression_type,										\
			typename et::expr<t2_, YT>::expression_type,										\
			oper_																				\
		>																						\
	>																							\
	operator oper_def_ (const et::expr<t1_, XT>& lhs, const et::expr<t2_, YT>& rhs)				\
	{																							\
		return et::make_expr( et::make_binary_expr<r_, oper_>									\
			(lhs.expression(), rhs.expression()) );												\
	}
	

//=====================================================================
} // namespace et
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
