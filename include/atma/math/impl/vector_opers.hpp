


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



