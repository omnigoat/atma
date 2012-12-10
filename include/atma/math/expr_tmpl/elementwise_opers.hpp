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

	//=====================================================================
	// addition
	//=====================================================================
	template <typename LHS, typename RHS>
	struct elementwise_add_oper
	{
		elementwise_add_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return value<LHS>::get(lhs, i) + value<RHS>::get(rhs, i); }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};


	//=====================================================================
	// subtraction
	//=====================================================================
	template <typename LHS, typename RHS>
	struct elementwise_sub_oper
	{
		elementwise_sub_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return value<LHS>::get(lhs, i) - value<RHS>::get(rhs, i); }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};


	//=====================================================================
	// multiplication
	//=====================================================================
	template <typename LHS, typename RHS>
	struct elementwise_mul_oper
	{
		elementwise_mul_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return value<LHS>::get(lhs, i) * value<RHS>::get(rhs, i); }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};


	//=====================================================================
	// division
	//=====================================================================
	template <typename LHS, typename RHS>
	struct elementwise_div_oper
	{
		elementwise_div_oper(const LHS& lhs, const RHS& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}

		typename element_type_of<LHS>::type
		 operator [](int i) const
		  { return value<LHS>::get(lhs, i) / value<RHS>::get(rhs, i); }

	private:
		typename member_type<LHS>::type lhs;
		typename member_type<RHS>::type rhs;
	};



//=====================================================================
} // namespace expr_tmpl
} // namespace atma
//=====================================================================
#endif
//=====================================================================
