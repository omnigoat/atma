#pragma once
//=====================================================================
#include <atma/math/impl/expr.hpp>
#include <atma/math/impl/storage_policy.hpp>
//=====================================================================
namespace atma { namespace math { namespace impl {

	template <typename R, template <typename, typename> class OPER, typename LHS, typename RHS>
	struct binary_expr : expr<R, OPER<LHS, RHS>>
	{
		binary_expr(LHS const& lhs, RHS const& rhs)
			: lhs(lhs), rhs(rhs)
		{
		}

		typename storage_policy<typename std::decay<LHS>::type>::type lhs;
		typename storage_policy<typename std::decay<RHS>::type>::type rhs;

		binary_expr& operator = (binary_expr const&) = delete;
	};

} } }
