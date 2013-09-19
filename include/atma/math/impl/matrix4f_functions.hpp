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

		__m128 d0 = _mm_dp_ps(r0.xmmd(), npos.xmmd(), 0xef);
		__m128 d1 = _mm_dp_ps(r1.xmmd(), npos.xmmd(), 0xef);
		__m128 d2 = _mm_dp_ps(r2.xmmd(), npos.xmmd(), 0xef);

		auto result = matrix4f(
			_am_select_ps(d0, r0.xmmd(), xmmd_mask_1110_ps),
			_am_select_ps(d1, r1.xmmd(), xmmd_mask_1110_ps),
			_am_select_ps(d2, r2.xmmd(), xmmd_mask_1110_ps),
			xmmd_identity_r3_ps
		);

		return result.transposed();
	}
	
	inline auto look_to(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f
	{
		return look_along(position, target - position, up);
	}

#undef near
#undef far
	inline auto pespective(float width, float height, float near, float far) -> matrix4f
	{
		float nn = near + near;
		float range = far / (far - near);

		__m128 rmem = _am_load_f32x4(nn / width, nn / height, range, -range * near);
		__m128 t0 = _mm_setzero_ps();

		__m128 r0 = _mm_move_ss(t0, rmem);
		__m128 r1 = _mm_and_ps(rmem, xmmd_mask_0100_ps);
		rmem = _mm_shuffle_ps(rmem, xmmd_identity_r3_ps, _MM_SHUFFLE(3, 2, 3, 2));
		__m128 r2 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(3, 0, 0, 0));
		__m128 r3 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(2, 1, 0, 0));

		return matrix4f{r0, r1, r2, r3};
	}

//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
