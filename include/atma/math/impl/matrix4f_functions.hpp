//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_MATRIX4F_FUNCTIONS_HPP
#define ATMA_MATH_IMPL_MATRIX4F_FUNCTIONS_HPP
//=====================================================================
#ifndef ATMA_MATH_MATRIX4F_SCOPE
#	error "this file needs to be included solely from matrix4.hpp"
#endif
//=====================================================================
#include <atma/math/impl/constants.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	


	// left-handed look-along view matrix
	inline auto look_along(vector4f const& position, vector4f const& direction, vector4f const& up) -> matrix4f
	{
		vector4f r2 = direction.normalized();
		vector4f r0 = cross_product(up, r2).normalized();
		vector4f r1 = cross_product(r2, r0);
		vector4f npos = vector4f(_mm_mul_ps(position.xmmd(), xmmd_negone_ps));

		__m128 d0 = _mm_dp_ps(r0.xmmd(), npos.xmmd(), 0x7f);
		__m128 d1 = _mm_dp_ps(r1.xmmd(), npos.xmmd(), 0x7f);
		__m128 d2 = _mm_dp_ps(r2.xmmd(), npos.xmmd(), 0x7f);

		auto result = matrix4f(
			_am_select_ps(d0, r0.xmmd(), xmmd_mask_1110_ps),
			_am_select_ps(d1, r1.xmmd(), xmmd_mask_1110_ps),
			_am_select_ps(d2, r2.xmmd(), xmmd_mask_1110_ps),
			xmmd_identity_r3_ps
		);

		return result.transposed();
	}
	
	inline auto look_at(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f
	{
		return look_along(position, target - position, up);
	}

#undef near
#undef far
	inline auto pespective(float width, float height, float near, float far) -> matrix4f
	{
		float nn = near + near;
		float range = far / (far - near);

		__m128 rmem = _am_load_ps(-range * near, range, nn / height, nn / width);
		__m128 t0 = _mm_setzero_ps();

		__m128 r0 = _mm_move_ss(t0, rmem);
		__m128 r1 = _mm_and_ps(rmem, xmmd_mask_0100_ps);
		rmem = _mm_shuffle_ps(rmem, xmmd_identity_r3_ps, _MM_SHUFFLE(3, 2, 3, 2));
		__m128 r2 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(3, 0, 0, 0));
		__m128 r3 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(2, 1, 0, 0));

		return matrix4f{r0, r1, r2, r3};
	}


	inline auto perspective_fov(float fov, float aspect, float near, float far) -> matrix4f
	{
#ifdef ATMA_MATH_USE_SSE
		float sin_fov, cos_fov;
		retrieve_sin_cos(sin_fov, cos_fov, 0.5f * fov);

		float range = far / (far-near);
		float scale = cos_fov / sin_fov;

		auto values = _am_load_ps(scale / aspect, scale, range, -range * near);
		// range,-range * near,0,1.0f
		auto values2 = _mm_shuffle_ps(values, xmmd_identity_r3_ps, _MM_SHUFFLE(3, 2, 3, 2));

		// scale/aspect,0,0,0
		return matrix4f(
			// cos(fov)/sin(fov), 0, 0, 0
			_mm_move_ss(xmmd_zero_ps, values),
			// 0, height/aspect, 0, 0
			_mm_and_ps(values, xmmd_mask_0100_ps),
			// 0, 0, range, 1.f
			_mm_shuffle_ps(xmmd_zero_ps, values2, _MM_SHUFFLE(3, 0, 0, 0)),
			// 0, 0, -range*near, 0
			_mm_shuffle_ps(xmmd_zero_ps, values2, _MM_SHUFFLE(2, 1, 0, 0))
		);
#else
		//
		// General form of the Projection Matrix
		//
		// uh = Cot( fov/2 ) == 1/Tan(fov/2)
		// uw / uh = 1/aspect
		// 
		//   uw         0       0       0
		//    0        uh       0       0
		//    0         0      f/(f-n)  1
		//    0         0    -fn/(f-n)  0
		//
		// Make result to be identity first

		// check for bad parameters to avoid divide by zero:
		// if found, assert and return an identity matrix.
		if (fov <= 0 || aspect == 0)
			ATMA_ASSERT(fov > 0 && aspect != 0);

		float depth = far - near;
		float one_over_depth = 1.f / depth;

		math::matrix4f result;
		result[1][1] = 1 / tan(0.5f * fov);
		result[0][0] = result[1][1] / aspect;
		result[2][2] = far * one_over_depth;
		result[3][2] = (-far * near) * one_over_depth;
		result[2][3] = 1;
		result[3][3] = 0;

		return result;
#endif
	}



	inline auto rotation_y(float angle) -> matrix4f
	{
#ifdef ATMA_MATH_USE_SSE
		float sin_angle;
		float cos_angle;
		retrieve_sin_cos(sin_angle, cos_angle, angle);

		auto sin = _mm_set_ss(sin_angle);
		auto cos = _mm_set_ss(cos_angle);
		
		return matrix4f(
			// cos,0,-sin,0
			_mm_shuffle_ps(cos, _mm_mul_ps(sin, xmmd_neg_x_ps), _MM_SHUFFLE(3, 0, 3, 0)),
			// 0,1,0,0
			xmmd_identity_r1_ps,
			// sin,0,cos,0
			_mm_shuffle_ps(sin, cos, _MM_SHUFFLE(3, 0, 3, 0)),
			// 0,0,0,1
			xmmd_identity_r3_ps
		);

#else
		float    fSinangle;
		float    fCosangle;
		XMScalarSinCos(&fSinangle, &fCosangle, angle);

		XMMATRIX M;
		M.m[0][0] = 1.0f;
		M.m[0][1] = 0.0f;
		M.m[0][2] = 0.0f;
		M.m[0][3] = 0.0f;

		M.m[1][0] = 0.0f;
		M.m[1][1] = fCosangle;
		M.m[1][2] = fSinangle;
		M.m[1][3] = 0.0f;

		M.m[2][0] = 0.0f;
		M.m[2][1] = -fSinangle;
		M.m[2][2] = fCosangle;
		M.m[2][3] = 0.0f;

		M.m[3][0] = 0.0f;
		M.m[3][1] = 0.0f;
		M.m[3][2] = 0.0f;
		M.m[3][3] = 1.0f;
		return M;
#endif
	}

	inline auto rotation_x(float angle) -> matrix4f
	{
#ifdef ATMA_MATH_USE_SSE
		float sin_angle;
		float cos_angle;
		retrieve_sin_cos(sin_angle, cos_angle, angle);

		auto sin = _mm_set_ss(sin_angle);
		auto cos = _mm_set_ss(cos_angle);

		return matrix4f(
			// 1,0,0,0
			xmmd_identity_r0_ps,
			// 0,cos,sin,0
			_mm_shuffle_ps(cos, sin, _MM_SHUFFLE(3, 0, 0, 3)),
			// 0,-sin,cos,0
			_mm_shuffle_ps(_mm_mul_ps(sin, xmmd_neg_x_ps), cos, _MM_SHUFFLE(3, 0, 0, 3)),
			// 0,0,0,1
			xmmd_identity_r3_ps
		);

#else
		float    fSinangle;
		float    fCosangle;
		XMScalarSinCos(&fSinangle, &fCosangle, angle);

		XMMATRIX M;
		M.m[0][0] = 1.0f;
		M.m[0][1] = 0.0f;
		M.m[0][2] = 0.0f;
		M.m[0][3] = 0.0f;

		M.m[1][0] = 0.0f;
		M.m[1][1] = fCosangle;
		M.m[1][2] = fSinangle;
		M.m[1][3] = 0.0f;

		M.m[2][0] = 0.0f;
		M.m[2][1] = -fSinangle;
		M.m[2][2] = fCosangle;
		M.m[2][3] = 0.0f;

		M.m[3][0] = 0.0f;
		M.m[3][1] = 0.0f;
		M.m[3][2] = 0.0f;
		M.m[3][3] = 1.0f;
		return M;
#endif
	}
//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
