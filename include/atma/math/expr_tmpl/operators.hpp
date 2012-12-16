//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_OPERATORS_HPP
#define ATMA_MATH_EXPR_TMPL_OPERATORS_HPP
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================
	
	//=====================================================================
	// operator type-to-type
	//=====================================================================
	#define ATMA_MATH_OPERATOR_T_T(fn, name, lhst, rhst, rt) \
	expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, rhst>>##fn##(const lhst##& lhs, const rhst##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, rhst>>(expr_tmpl::##name##<lhst, rhst>(lhs, rhs)); \
	}
	
	//=====================================================================
	// operator type-to-expr
	//=====================================================================
	#define ATMA_MATH_OPERATOR_T_X(fn, name, lhst, rhst, rt) \
	template <typename RHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, expr_tmpl::expr<rhst, RHS_OPER>>>##fn##(const lhst##& lhs, const expr_tmpl::expr<rhst, RHS_OPER>& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<lhst, expr_tmpl::expr<rhst, RHS_OPER>>>(expr_tmpl::##name##<lhst, expr_tmpl::expr<rhst, RHS_OPER>>(lhs, rhs)); \
	}

	//=====================================================================
	// operator expr-to-type
	//=====================================================================
	#define ATMA_MATH_OPERATOR_X_T(fn, name, lhst, rhst, rt) \
	template <typename LHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, rhst>>##fn##(const expr_tmpl::expr<lhst, LHS_OPER>& lhs, const rhst##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, rhst>> \
		 (expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, rhst>(lhs, rhs)); \
	}
	
	//=====================================================================
	// operator expr-to-expr
	//=====================================================================
	#define ATMA_MATH_OPERATOR_X_X(fn, name, lhst, rhst, rt) \
	template <typename LHS_OPER, typename RHS_OPER> \
	expr_tmpl::expr<rt, expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, expr_tmpl::expr<rhst, RHS_OPER>>> \
	fn##(const expr_tmpl::expr<lhst, LHS_OPER>& lhs, const expr_tmpl::expr<rhst, RHS_OPER>##& rhs) { \
		return expr_tmpl::expr<rt, expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, expr_tmpl::expr<rhst, RHS_OPER>>> \
		(expr_tmpl::##name##<expr_tmpl::expr<lhst, LHS_OPER>, expr_tmpl::expr<rhst, RHS_OPER>>(lhs, rhs)); \
	}


	//=====================================================================
	// less typing!
	//=====================================================================
	#define ATMA_MATH_OPERATOR_T_TX(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_T_T(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_T_X(fn, name, lhst, rhst, rt)

	#define ATMA_MATH_OPERATOR_TX_T(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_T_T(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_X_T(fn, name, lhst, rhst, rt)

	#define ATMA_MATH_OPERATOR_TX_TX(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_T_T(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_T_X(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_X_T(fn, name, lhst, rhst, rt) \
		ATMA_MATH_OPERATOR_X_X(fn, name, lhst, rhst, rt)
	

//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
