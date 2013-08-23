
//=====================================================================
// expression template functions
//
// this is where the operators actually do the work on per-element
//=====================================================================
namespace impl
{
	struct vector_add {};
	struct vector_sub {};
	struct vector_mul_post {};
	struct vector_mul_pre {};
	struct vector_div {};


	// vector_add
	template <typename LHS, typename RHS>
	struct expr<vector4f, vector_add, LHS, RHS> : base_expr<vector4f>
	{
		expr(LHS const& lhs, RHS const& rhs)
		: lhs(lhs), rhs(rhs)
		{
#ifdef ATMA_MATH_USE_SSE
			xmmd_ = _mm_add_ps(lhs.xmmd_, rhs.xmmd_);
#endif
		}
		
#ifndef ATMA_MATH_USE_SSE
		auto operator [](uint32_t i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] + rhs[i];
		}
#endif
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	// vector_sub
	template <typename LHS, typename RHS>
	struct expr<vector4f, vector_sub, LHS, RHS> : base_expr<vector4f>
	{
		expr(LHS const& lhs, RHS const& rhs)
		: lhs(lhs), rhs(rhs)
		{
#ifdef ATMA_MATH_USE_SSE
			xmmd_ = _mm_sub_ps(lhs.xmmd_, rhs.xmmd_);
#endif
		}
	
#ifndef ATMA_MATH_USE_SSE
		auto operator [](uint32_t i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] - rhs[i];
		}
#endif
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	// vector_mul_post
	template <typename LHS, typename RHS>
	struct expr<vector4f, vector_mul_post, LHS, RHS> : base_expr<vector4f>
	{
		expr(LHS const& lhs, RHS const& rhs)
		: lhs(lhs), rhs(rhs)
		{
#ifdef ATMA_MATH_USE_SSE
			__m128 const scalar = _mm_set1_ps(rhs);
			xmmd_ = _mm_mul_ps(lhs.xmmd_, scalar);
#endif
		}

#ifndef ATMA_MATH_USE_SSE
		auto operator[](uint32_t i) const->T {
			ATMA_ASSERT(i < E);
			return lhs[i] * rhs[i];
		}
#endif
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

#if 0
	// vector_mul_pre
	template <typename LHS, typename RHS>
	struct expr<vector4f, binary_oper<vector_mul_pre, LHS, RHS>
	{
		expr(LHS const& lhs, RHS const& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](uint32_t i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs * rhs[i];
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};

	// vector_div
	template <typename LHS, typename RHS>
	struct expr<vector4f, binary_oper<vector_div, LHS, RHS>
	{
		expr(LHS const& lhs, RHS const& rhs)
		 : lhs(lhs), rhs(rhs)
		  {}
		
		auto operator [](uint32_t i) const -> T {
			ATMA_ASSERT(i < E);
			return lhs[i] / rhs;
		}
	
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	};
#endif

}
	



//=====================================================================
// addition
//=====================================================================
// T + T
inline auto operator + (vector4f const& lhs, vector4f const& rhs) ->
impl::expr<vector4f, impl::vector_add, vector4f, vector4f> {
	return impl::expr<vector4f, impl::vector_add, vector4f, vector4f>(lhs, rhs);
}

// T + X
template <typename OPER>
inline auto operator + (vector4f const& lhs, impl::expr<vector4f, OPER> const& rhs) ->
impl::expr<vector4f, impl::vector_add, vector4f, impl::expr<vector4f, OPER>> {
	return impl::expr<vector4f, impl::vector_add, vector4f, impl::expr<vector4f, OPER>>(lhs, rhs);
}

// X + T
template <typename OPER>
inline auto operator + (impl::expr<vector4f, OPER> const& lhs, vector4f const& rhs) ->
impl::expr<vector4f, impl::vector_add, impl::expr<vector4f, OPER>, vector4f> {
	return impl::expr<vector4f, impl::vector_add, impl::expr<vector4f, OPER>, vector4f>(lhs, rhs);
}

// X + X
template <typename LHS_OPER, typename RHS_OPER>
inline auto operator + (impl::expr<vector4f, LHS_OPER>& lhs, impl::expr<vector4f, RHS_OPER>& rhs) ->
impl::expr<vector4f, impl::vector_add, impl::expr<vector4f, LHS_OPER>, impl::expr<vector4f, RHS_OPER>> {
	return impl::expr<vector4f, impl::vector_add, impl::expr<vector4f, LHS_OPER>, impl::expr<vector4f, RHS_OPER>>(lhs, rhs);
}



//=====================================================================
// subtraction
//=====================================================================
// T - T
inline auto operator - (vector4f const& lhs, vector4f const& rhs) ->
impl::expr<vector4f, impl::vector_sub, vector4f, vector4f> {
	return impl::expr<vector4f, impl::vector_sub, vector4f, vector4f>(lhs, rhs);
}

// T - X
template <typename OPER>
inline auto operator - (vector4f const& lhs, impl::expr<vector4f, OPER> const& rhs) ->
impl::expr<vector4f, impl::vector_sub, vector4f, impl::expr<vector4f, OPER>> {
	return impl::expr<vector4f, impl::vector_sub, vector4f, impl::expr<vector4f, OPER>>(lhs, rhs);
}
	
// X - T
template <typename OPER>
inline auto operator - (impl::expr<vector4f, OPER> const& lhs, vector4f const& rhs) ->
impl::expr<vector4f, impl::vector_sub, impl::expr<vector4f, OPER>, vector4f> {
	return impl::expr<vector4f, impl::vector_sub, impl::expr<vector4f, OPER>, vector4f>(lhs, rhs);
}

// X - X
template <typename LHS_OPER, typename RHS_OPER>
inline auto operator - (impl::expr<vector4f, LHS_OPER>& lhs, impl::expr<vector4f, RHS_OPER>& rhs) ->
impl::expr<vector4f, impl::vector_sub, impl::expr<vector4f, LHS_OPER>, impl::expr<vector4f, RHS_OPER>> {
	return impl::expr<vector4f, impl::vector_sub, impl::expr<vector4f, LHS_OPER>, impl::expr<vector4f, RHS_OPER>>(lhs, rhs);
}



//=====================================================================
// post multiplication
//    note: we don't have an expr for the scalar, because any expression
//          that results in a single element almost definitely has high
//          computational costs (like dot-products)
//=====================================================================
// T * T
inline auto operator * (vector4f const& lhs, const T& rhs) ->
impl::expr<vector4f, impl::vector_mul_post, vector4f, T> {
	return impl::expr<vector4f, impl::vector_mul_post, vector4f, T>(lhs, rhs);
}

// X * T
template <typename OPER>
inline auto operator * (impl::expr<vector4f, OPER> const& lhs, const T& rhs) ->
impl::expr<vector4f, impl::vector_mul_post, impl::expr<vector4f, OPER>, T> {
	return impl::expr<vector4f, impl::vector_mul_post, impl::expr<vector4f, OPER>, T>(lhs, rhs);
}

//=====================================================================
// pre multiplication
//=====================================================================
#if 0
// T * T
inline auto operator * (const T& lhs, vector4f const& rhs) ->
impl::expr<vector4f, impl::vector_mul_pre, T, vector4f> {
	return impl::expr<vector4f, impl::vector_mul_pre, T, vector4f>(lhs, rhs);
}

// T * X
template <typename OPER>
inline auto operator * (const T& lhs, impl::expr<vector4f, OPER> const& rhs) ->
impl::expr<vector4f, impl::vector_mul_pre, T, impl::expr<vector4f, OPER>> {
	return impl::expr<vector4f, impl::vector_mul_pre, T, impl::expr<vector4f, OPER>>(lhs, rhs);
}
#endif

