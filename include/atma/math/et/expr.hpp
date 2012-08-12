//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_EXPR_HPP
#define ATMA_MATH_ET_EXPR_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
#include <atma/math/et/unary_expr.hpp>
#include <atma/math/et/binary_expr.hpp>
#include "operators.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et {
//=====================================================================

	template <typename R, typename EXPR_T>
	struct expr
	{
		typedef typename EXPR_T::component_type component_type;
		typedef EXPR_T expression_type;
		
		explicit expr(const EXPR_T& x)
		 : x(x)
		{
		}
		
		typename component_type operator ()(int i) const
		{
			return x(i);
		}
		
		const EXPR_T& expression() const
		{
			return x;
		}
		
	private:
		const EXPR_T x;
	};

	template <typename EXPR_T>
	expr<typename EXPR_T::result_type, EXPR_T> make_expr(const EXPR_T& x)
	{
		return expr<typename EXPR_T::result_type, EXPR_T>(x);
	}
	
	// to unambiguate expr-something-expr
	/*
	template <typename LHS_EXPR, typename RHS_EXPR>
	et::expr< et::binary_expr<et::expr<LHS_EXPR>, et::expr<RHS_EXPR>, et::add_oper> > operator + 
		(const et::expr<LHS_EXPR>& lhs, const et::expr<RHS_EXPR>& rhs)
	{
		return et::make_expr( et::make_binary_expr<et::add_oper>(lhs, rhs) );
	}
	*/
	
//=====================================================================
} // namespace et
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
