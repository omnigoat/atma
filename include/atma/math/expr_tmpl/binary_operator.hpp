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

	template <template <typename, typename> class TAG, typename LHS, typename RHS>
	struct binary_oper
	{
		binary_oper(const LHS& lhs, const RHS& rhs)
		 : lhs_(lhs), rhs_(rhs)
		  {}
		
		typename element_type_of<TAG<LHS, RHS>>::type
		 operator [](int i) const
		  { return tag_(lhs_, rhs_, i); }

	private:
		TAG<LHS, RHS> tag_;
		const LHS& lhs_;
		const RHS& rhs_;
	};


	template <typename LHS, typename RHS>
	struct add_oper {
		typedef decltype(std::declval<LHS>()[0] + std::declval<RHS>()[0]) result_type;

		result_type operator ()(const LHS& lhs, const RHS& rhs, int i) const
		 { return lhs[i] + rhs[i]; }
	};

//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
