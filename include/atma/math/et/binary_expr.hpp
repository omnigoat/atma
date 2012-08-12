//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_BINARY_EXPR_HPP
#define ATMA_MATH_ET_BINARY_EXPR_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
#include <atma/math/vector.hpp>
#include <atma/math/et/expr_traits.hpp>
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et {
//=====================================================================


	template <typename R, typename T1, typename T2, template <typename, typename, typename> class Op>
	struct binary_expr
	{
	public:
		// some traits for everyone
		typedef binary_expr_traits<R, T1, T2> traits;
		
		// the operator's type
		typedef Op<R, T1, T2> operator_type;
		// the logical type that is return from this operator
		typedef typename traits::result_type result_type;
		// the component type (used in operator())
		typedef typename traits::lhs_component_type component_type;
		
	public:		
		// our constructor
		explicit binary_expr
			(typename traits::lhs_const_reference_type lhs, typename traits::rhs_const_reference_type rhs)
		 : lhs(lhs), rhs(rhs), oper(lhs, rhs)
		{
		}
		
		// accessing data
		component_type operator()(int i) const
		{
			return oper(i);
		}
				
		
	public:
		// const references, okay to be public
		typename traits::lhs_const_reference_type lhs;
		typename traits::rhs_const_reference_type rhs;
	
	private:
		operator_type oper;
	};

	template <typename R, template <class, class, class> class Op, typename LHS, typename RHS>
	inline binary_expr<R, LHS, RHS, Op> make_binary_expr(const LHS& lhs, const RHS& rhs)
	{
		return binary_expr<R, LHS, RHS, Op>(lhs, rhs);
	}

/*
binary_expr<vector3, vector3, plus_oper> operator + (const vector3& lhs, const vector3& rhs)
{
	return make_expr( binary_expr<vector3, vector3, plus_oper>(lhs, rhs) );
}
*/

/*
binary_expr<vector3, vector3, sub_oper> operator -
	(const vector3& lhs, const vector3& rhs)
{
	return binary_expr<vector3, vector3, sub_oper>(lhs, rhs);
}

template <typename LHS_OP>
binary_expr<vector3, vector3, sub_oper> operator -
	(const binary_expr<vector3, vector3, LHS_OP>& lhs, const vector3& rhs)
{
	return binary_expr<vector3, vector3, sub_oper>(lhs, rhs);
}

template <typename RHS_OP>
binary_expr<vector3, vector3, sub_oper> operator -
	(const vector3& lhs, const binary_expr<vector3, vector3, RHS_OP>& rhs)
{
	return binary_expr<vector3, vector3, sub_oper>(lhs, rhs);
}

template <typename LHS_OP, typename RHS_OP>
binary_expr<vector3, vector3, sub_oper> operator - 
	(const binary_expr<vector3, vector3, LHS_OP>& lhs, const binary_expr<vector3, vector3, RHS_OP>& rhs)
{
	return binary_expr<vector3, vector3, sub_oper>(lhs, rhs);
}
*/

//=====================================================================
} // namespace et
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
