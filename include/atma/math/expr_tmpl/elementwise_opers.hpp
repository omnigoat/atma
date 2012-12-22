//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_ELEMENTWISE_OPERS_HPP
#define ATMA_MATH_EXPR_TMPL_ELEMENTWISE_OPERS_HPP
//=====================================================================
#include <atma/math/expr_tmpl/utility.hpp>
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================

	template <typename LHS, typename RHS>
	struct elementwise_add
	{
		typedef decltype(std::declval<LHS>()[0] + std::declval<RHS>()[0]) result_type;

		result_type
		 operator ()(const LHS& lhs, const RHS& rhs, int i) const
		  { return lhs[i] + rhs[i]; }
	};


//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
