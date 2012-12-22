//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_BINARY_OPERATOR_HPP
#define ATMA_MATH_EXPR_TMPL_BINARY_OPERATOR_HPP
//=====================================================================
#include <atma/math/expr_tmpl/utility.hpp>
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================

	template <template <typename, typename> class FN, typename LHS, typename RHS>
	struct binary_oper
	{
		typedef FN<value_t<LHS>, value_t<RHS>> fn_t;
		typedef value_t<LHS> lhs_t;
		typedef value_t<RHS> rhs_t;

		binary_oper(const LHS& lhs, const RHS& rhs)
		 : lhs_(lhs), rhs_(rhs)
		  {}
		
		auto operator [](int i) const
		 -> decltype(fn_t()(std::declval<lhs_t>(), std::declval<rhs_t>(), i))
		  { return fn_(lhs_, rhs_, i); }

	private:
		fn_t fn_;
		lhs_t lhs_;
		rhs_t rhs_;
	};






//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
