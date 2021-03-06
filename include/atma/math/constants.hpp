#pragma once

#pragma warning(push,3)
#include <xmmintrin.h>
#pragma warning(pop)
#undef min
#undef max
#undef near
#undef far


namespace atma { namespace math {

	float const pi = 3.141592654f;
	float const two_pi = 6.283185307f;
	float const one_over_pi = 0.318309886f;
	float const one_over_two_pi = 0.159154943f;
	float const pi_over_two = 1.570796327f;
	float const pi_over_four = 0.785398163f;

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

	__m128 const xmmd_neg_x_ps = _mm_setr_ps(-1.f, 0.f, 0.f, 0.f);
	__m128 const xmmd_neg_y_ps = _mm_setr_ps(0.f, -1.f, 0.f, 0.f);
	__m128 const xmmd_neg_z_ps = _mm_setr_ps(0.f, 0.f, -1.f, 0.f);
	__m128 const xmmd_neg_w_ps = _mm_setr_ps(0.f, 0.f, 0.f, -1.f);


	// SERIOUSLY, find a better spot for this
	inline auto _am_select_ps(__m128 a, __m128 b, __m128 mask) -> __m128
	{
		return _mm_or_ps(_mm_andnot_ps(mask, a), _mm_and_ps(b, mask));
	}

	inline auto _am_load_ps(float f0, float f1, float f2, float f3) -> __m128
	{
		__declspec(align(16)) float fs[] = {f0, f1, f2, f3};
		return _mm_load_ps(fs);
	}

} }
