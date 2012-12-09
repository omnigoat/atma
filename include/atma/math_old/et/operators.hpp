//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_OPERATORS_HPP
#define ATMA_MATH_ET_OPERATORS_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
#include <atma/math/et/expr_traits.hpp>
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et {
//=====================================================================

	
	//=====================================================================
	// creating an operator
	//=====================================================================
	#define ATMA_MATH_ET_OPERATOR(oper_name_, oper_)						\
	template <typename R, typename T1, typename T2>							\
	struct oper_name_														\
	{																		\
		typedef binary_expr_traits<R, T1, T2> traits;						\
		typedef typename traits::result_type result_type;					\
		typedef typename traits::result_component_type component_type;		\
																			\
		explicit oper_name_													\
			(																\
				typename traits::lhs_const_reference_type lhs,				\
				typename traits::rhs_const_reference_type rhs				\
			)																\
		 : lhs(lhs), rhs(rhs)												\
		{																	\
		}																	\
																			\
		component_type operator ()(int i) const								\
		{																	\
			return traits::get_lhs(lhs, i) oper_ traits::get_rhs(rhs, i);	\
		}																	\
																			\
	private:																\
		typename traits::lhs_const_reference_type lhs;						\
		typename traits::rhs_const_reference_type rhs;						\
	};
	
	ATMA_MATH_ET_OPERATOR(add_oper, +)
	ATMA_MATH_ET_OPERATOR(sub_oper, -)
	ATMA_MATH_ET_OPERATOR(mul_oper, *)
	ATMA_MATH_ET_OPERATOR(div_oper, /)

//=====================================================================
} // namespace et	
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
