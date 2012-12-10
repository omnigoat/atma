//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_EXPR_TMPL_EXPRESSION_HPP
#define ATMA_MATH_EXPR_TMPL_EXPRESSION_HPP
//=====================================================================
#include <atma/math/expr_tmpl/utility.hpp>
//=====================================================================
namespace atma {
namespace expr_tmpl {
//=====================================================================

	template <typename R, typename OPER>
	struct expr
	{
		expr(const OPER& oper)
		 : oper(oper)
		 {
		 }

		typename element_type_of<OPER>::type
		 operator [](int i) const
		  { return oper[i]; }

	private:
		OPER oper;
	};


//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
