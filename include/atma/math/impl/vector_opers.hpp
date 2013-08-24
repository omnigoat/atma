

#ifdef ATMA_MATH_USE_SSE
#define OPERATOR(name, fpu_fn, sse_fn) \
	template <typename LHS, typename RHS> \
	struct name : expr<vector4f, name<LHS, RHS>> \
	{ \
		name(LHS const& lhs, RHS const& rhs) \
		: lhs(lhs), rhs(rhs) \
		{} \
		auto xmmd() const -> __m128 \
		sse_fn \
		typename storage_policy<LHS>::type lhs; \
		typename storage_policy<RHS>::type rhs; \
	}
#else
#define OPERATOR(name, fpu_fn, sse_fn) \
	template <typename LHS, typename RHS> \
	struct name : expr<vector4f, name<LHS, RHS>> \
	{ \
		name(LHS const& lhs, RHS const& rhs) \
		: lhs(lhs), rhs(rhs) \
		{} \
		auto operator [](uint32_t i) const -> float \
		fpu_fn \
		typename storage_policy<LHS>::type lhs; \
		typename storage_policy<RHS>::type rhs; \
	}
#endif

//=====================================================================
// expression template functions
//
// this is where the operators actually do the work on per-element
//=====================================================================
namespace impl
{
	auto xmmd_of(vector4f const& vector) -> __m128
	{
		return vector.xmmd();
	}

	template <typename OP>
	auto xmmd_of(expr<vector4f, OP> const& expr) -> __m128
	{
		return expr.xmmd();
	}

	auto xmmd_of(float x) -> __m128
	{
		return _mm_load_ps1(&x);
	}

	template <typename T>
	auto element_of(T const& x, uint32_t i) -> float
	{
		return x[i];
	}

	auto element_of(float x, uint32_t) -> float
	{
		return x;
	}

	OPERATOR(vector4f_add,
		{ return element_of(lhs, i) + element_of(rhs, i); },
		{ return _mm_add_ps(xmmd_of(lhs), xmmd_of(rhs)); }
	);

	OPERATOR(vector4f_sub,
		{ return element_of(lhs, i) - element_of(rhs, i); },
		{ return _mm_sub_ps(xmmd_of(lhs), xmmd_of(rhs)); }
	);

	OPERATOR(vector4f_mul,
		{ return element_of(lhs, i) * element_of(rhs, i); },
		{ return _mm_mul_ps(xmmd_of(lhs), xmmd_of(rhs)); }
	);

	OPERATOR(vector4f_div,
		{ return element_of(lhs, i) / element_of(rhs);},
		{ return _mm_div_ps(xmmd_of(lhs), xmmd_of(rhs)); }
	);

#if ATMA_MATH_USE_AVX
	// whoo, multiply-add
	template <typename ALHS, typename MLHS, typename MRHS>
	struct vector4f_fmadd<ALHS, vector4f_mul<MLHS, MRHS>>
	{
		vector4f_fmadd(ALHS const& lhs, vector4f_mul<MLHS, MRHS> const& rhs)
		: lhs(lhs), rhs(rhs)
		{}

		auto xmmd() const -> __m128
		{
			return xmmd_of(lhs)
		}
		
		typename storage_policy<LHS>::type lhs;
		typename storage_policy<RHS>::type rhs;
	}
#endif

}

#undef OPERATOR




//=====================================================================
// addition
//=====================================================================
// expr + expr
template <typename LOP, typename ROP>
inline auto operator + (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
	-> impl::vector4f_add<decltype(lhs), decltype(rhs)>
{
	return { lhs, rhs };
}



//=====================================================================
// subtraction
//=====================================================================
// expr - expr
template <typename LOP, typename ROP>
inline auto operator - (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
	-> impl::vector4f_sub<decltype(lhs), decltype(rhs)>
{
	return { lhs, rhs };
}


//=====================================================================
// multiplication
//    note: we don't have an expr for the scalar, because any expression
//          that results in a single element almost definitely has high
//          computational costs (like dot-products)
//=====================================================================
// X * float
template <typename OPER>
inline auto operator * (impl::expr<vector4f, OPER> const& lhs, float rhs)
	-> impl::vector4f_mul<impl::expr<vector4f, OPER>, float>
{
	return {lhs, rhs};
}

// float * X
template <typename OPER>
inline auto operator * (float lhs, impl::expr<vector4f, OPER> const& rhs)
	-> impl::vector4f_mul<float, impl::expr<vector4f, OPER>>
{
	return {lhs, rhs};
}

//=====================================================================
// division
//=====================================================================
// X / float
template <typename OPER>
inline auto operator / (impl::expr<vector4f, OPER> const& lhs, float rhs)
	-> impl::vector4f_div<impl::expr<vector4f, OPER>, float>
{
	return { lhs, rhs };
}

