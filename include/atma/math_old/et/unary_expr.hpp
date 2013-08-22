//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_ET_UNARY_EXPR_HPP
#define ATMA_MATH_ET_UNARY_EXPR_HPP
//=====================================================================
#include <atma/math/math__core.hpp>
#include <atma/math/et/expr_traits.hpp>
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
namespace et {
//=====================================================================


	template <typename T, template <typename> class Op>
	struct unary_expr
	{
	public:
		// traits for our T
		typedef unary_expr_traits<T> traits;
		// the operator's type
		typedef Op<T> operator_type;
		// what we return
		typedef typename operator_type::result_type result_type;
	
	public:
		// our constructor
		explicit unary_expr
			(typename traits::const_reference_type input)
		 : oper(input)
		{
		}
		
		// accessing data
		result_type operator()(int i) const
		{
			return oper(i);
		}
		
	private:
		operator_type oper;
	};


//=====================================================================
} // namespace et
//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
