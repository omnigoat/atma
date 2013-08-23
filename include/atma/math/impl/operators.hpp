//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_impl_OPERATORS_HPP
#define ATMA_MATH_impl_OPERATORS_HPP
//=====================================================================
namespace atma {
namespace math {
namespace impl {
//=====================================================================
	
	//=====================================================================
	// operator type-to-type
	//=====================================================================
	#define ATMA_MATH_OPERATOR_T_T(fn, name, lhst, rhst, rt) \
	impl::expr<rt, impl::##name##<lhst, rhst>>##fn##(const lhst##& lhs, const rhst##& rhs) { \
		return impl::expr<rt, impl::##name##<lhst, rhst>>(impl::##name##<lhst, rhst>(lhs, rhs)); \
	}
	
	//=====================================================================
	// operator type-to-expr
	//=====================================================================
	#define ATMA_MATH_OPERATOR_T_X(fn, name, lhst, rhst, rt) \
	template <typename RHS_OPER> \
	impl::expr<rt, impl::##name##<lhst, impl::expr<rhst, RHS_OPER>>>##fn##(const lhst##& lhs, const impl::expr<rhst, RHS_OPER>& rhs) { \
		return impl::expr<rt, impl::##name##<lhst, impl::expr<rhst, RHS_OPER>>>(impl::##name##<lhst, impl::expr<rhst, RHS_OPER>>(lhs, rhs)); \
	}

	//=====================================================================
	// operator expr-to-type
	//=====================================================================
	#define ATMA_MATH_OPERATOR_X_T(fn, name, lhst, rhst, rt) \
	template <typename LHS_OPER> \
	impl::expr<rt, impl::##name##<impl::expr<lhst, LHS_OPER>, rhst>>##fn##(const impl::expr<lhst, LHS_OPER>& lhs, const rhst##& rhs) { \
		return impl::expr<rt, impl::##name##<impl::expr<lhst, LHS_OPER>, rhst>> \
		 (impl::##name##<impl::expr<lhst, LHS_OPER>, rhst>(lhs, rhs)); \
	}
	
	//=====================================================================
	// operator expr-to-expr
	//=====================================================================
	#define ATMA_MATH_OPERATOR_X_X(fn, name, lhst, rhst, rt) \
	template <typename LHS_OPER, typename RHS_OPER> \
	impl::expr<rt, impl::##name##<impl::expr<lhst, LHS_OPER>, impl::expr<rhst, RHS_OPER>>> \
	fn##(const impl::expr<lhst, LHS_OPER>& lhs, const impl::expr<rhst, RHS_OPER>##& rhs) { \
		return impl::expr<rt, impl::##name##<impl::expr<lhst, LHS_OPER>, impl::expr<rhst, RHS_OPER>>> \
		(impl::##name##<impl::expr<lhst, LHS_OPER>, impl::expr<rhst, RHS_OPER>>(lhs, rhs)); \
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
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
