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
		template <typename... Args>
		expr(const Args&... args)
		 : oper(args...)
		  {}
 
		auto operator [](int i) const
		 -> decltype(std::declval<OPER>()[i])
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
