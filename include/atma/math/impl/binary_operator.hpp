//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_BINARY_OPERATOR_HPP
#define ATMA_MATH_EXPR_TMPL_BINARY_OPERATOR_HPP
//=====================================================================
#include <atma/math/impl/utility.hpp>
//=====================================================================
namespace atma {
namespace math {
namespace expr_tmpl {
//=====================================================================

	template <typename FN, typename LHS, typename RHS>
	struct binary_oper
	{
		binary_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](int i) const
		 -> decltype(std::declval<FN>()(std::declval<LHS>(), std::declval<RHS>(), i))
		  { return fn_(lhs, rhs, i); }
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;

	private:
		FN fn_;
	};
		
//=====================================================================
} // namespace expr_tmpl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
