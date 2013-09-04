//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_CONSTANTS_HPP
#define ATMA_MATH_IMPL_CONSTANTS_HPP
//=====================================================================
#ifdef ATMA_MATH_USE_SSE
#include <xmmintrin.h>
#endif
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
#ifdef ATMA_MATH_USE_SSE

	__m128 const xmmd_zero_ps = _mm_setzero_ps();
	__m128 const xmmd_one_ps = _mm_setr_ps(1.f, 1.f, 1.f, 1.f);
	__m128 const xmmd_negone_ps = _mm_setr_ps(-1.f, -1.f, -1.f, -1.f);
	
	__m128 const xmmd_mask_1000_ps = _mm_cmplt_ps(xmmd_zero_ps, {1, 0, 0, 0});
	__m128 const xmmd_mask_0100_ps = _mm_cmplt_ps(xmmd_zero_ps, {0, 1, 0, 0});
	__m128 const xmmd_mask_0010_ps = _mm_cmplt_ps(xmmd_zero_ps, {0, 0, 1, 0});
	__m128 const xmmd_mask_0001_ps = _mm_cmplt_ps(xmmd_zero_ps, {0, 0, 0, 1});

	__m128 const xmmd_mask_1110_ps = _mm_cmplt_ps(xmmd_zero_ps, {1, 1, 1, 0});

	__m128 const xmmd_identity_r0_ps = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
	__m128 const xmmd_identity_r1_ps = _mm_setr_ps(0.f, 1.f, 0.f, 0.f);
	__m128 const xmmd_identity_r2_ps = _mm_setr_ps(0.f, 0.f, 1.f, 0.f);
	__m128 const xmmd_identity_r3_ps = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);


	// SERIOUSLY, find a better spot for this
	inline auto _am_select_ps(__m128 const& a, __m128 const& b, __m128 const& mask) -> __m128
	{
		return _mm_or_ps(_mm_andnot_ps(mask, a), _mm_and_ps(b, mask));
	}

	inline auto _am_load_f32x4(float f0, float f1, float f2, float f3) -> __m128
	{
		__declspec(align(16)) float fs[] = {f3, f2, f1, f0};
		return _mm_load_ps(fs);
	}

#endif
//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
