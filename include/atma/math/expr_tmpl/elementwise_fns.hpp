//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_ELEMENTWISE_FNS_HPP
#define ATMA_MATH_EXPR_TMPL_ELEMENTWISE_FNS_HPP
//=====================================================================
#include <atma/math/expr_tmpl/utility.hpp>
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================

	struct elementwise_add
	{
		template <typename LHS, typename RHS>
		auto operator ()(const LHS& lhs, const RHS& rhs, int i) const
		 -> decltype(lhs[i]+rhs[i])
		  { return lhs[i] + rhs[i]; }
	};
	
	struct elementwise_sub
	{
		template <typename LHS, typename RHS>
		auto operator ()(const LHS& lhs, const RHS& rhs, int i) const
		 -> decltype(lhs[i]-rhs[i])
		  { return lhs[i] - rhs[i]; }
	};

//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
