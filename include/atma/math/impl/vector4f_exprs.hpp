//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4F_EXPRS_HPP
#define ATMA_MATH_IMPL_VECTOR4F_EXPRS_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4F_SCOPE
#	error "this file needs to be included solely from vector4f.hpp"
#endif
//=====================================================================
#include <atma/math/impl/binary_expr.hpp>
//=====================================================================
namespace atma {
namespace math {
namespace impl {
//=====================================================================

	// vector4f_add
	template <typename LHS, typename RHS>
	struct vector4f_add : binary_expr<vector4f, vector4f_add, LHS, RHS>
	{
		vector4f_add(LHS const& lhs, RHS const& rhs) : binary_expr(lhs, rhs) {}

#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 { return _mm_add_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
		auto element(uint32 i) const -> float { return element_of(lhs, i) + element_of(rhs, i); }
#endif
	};

	// vector4f_sub
	template <typename LHS, typename RHS>
	struct vector4f_sub : binary_expr<vector4f, vector4f_sub, LHS, RHS>
	{
		vector4f_sub(LHS const& lhs, RHS const& rhs) : binary_expr(lhs, rhs) {}

#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 { return _mm_sub_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
		auto element(uint32 i) const -> float { return element_of(lhs, i) - element_of(rhs, i); }
#endif
	};

	// vector4f_mul
	template <typename LHS, typename RHS>
	struct vector4f_mul : binary_expr<vector4f, vector4f_mul, LHS, RHS>
	{
		vector4f_mul(LHS const& lhs, RHS const& rhs) : binary_expr(lhs, rhs) {}

#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 { return _mm_mul_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
		auto element(uint32 i) const -> float { return element_of(lhs, i) * element_of(rhs, i); }
#endif
	};

	// vector4f_div
	template <typename LHS, typename RHS>
	struct vector4f_div : binary_expr<vector4f, vector4f_div, LHS, RHS>
	{
		vector4f_div(LHS const& lhs, RHS const& rhs) : binary_expr(lhs, rhs) {}

#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 { return _mm_div_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
		auto element(uint32 i) const -> float { return element_of(lhs, i) / element_of(rhs, i); }
#endif
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
