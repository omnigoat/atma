//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_MATRIX4F_DEFINITION_HPP
#define ATMA_MATH_IMPL_MATRIX4F_DEFINITION_HPP
//=====================================================================
#include <atma/math/impl/constants.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	matrix4f::matrix4f()
	{
	}

	matrix4f::matrix4f(matrix4f const& rhs)
	{
		rmd_[0] = rhs.rmd_[0];
		rmd_[1] = rhs.rmd_[1];
		rmd_[2] = rhs.rmd_[2];
		rmd_[3] = rhs.rmd_[3];
	}

#ifdef ATMA_MATH_USE_SSE
	matrix4f::matrix4f(__m128 const& r0, __m128 const& r1, __m128 const& r2, __m128 const& r3)
	{
		rmd_[0] = r0;
		rmd_[1] = r1;
		rmd_[2] = r2;
		rmd_[3] = r3;
	}

	auto matrix4f::xmmd(uint32_t i) const -> __m128 const&
	{
		return rmd_[i];
	}
#endif

	

	inline auto operator * (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
#ifdef ATMA_MATH_USE_SSE
		auto r00 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(0), lhs.xmmd(0), _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmd(0));
		auto r01 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(0), lhs.xmmd(0), _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmd(1));
		auto r02 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(0), lhs.xmmd(0), _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmd(2));
		auto r03 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(0), lhs.xmmd(0), _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmd(3));
		auto r0 = _mm_add_ps(r00, _mm_add_ps(r01, _mm_add_ps(r02, r03)));

		auto r10 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(1), lhs.xmmd(1), _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmd(0));
		auto r11 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(1), lhs.xmmd(1), _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmd(1));
		auto r12 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(1), lhs.xmmd(1), _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmd(2));
		auto r13 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(1), lhs.xmmd(1), _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmd(3));
		auto r1 = _mm_add_ps(r10, _mm_add_ps(r11, _mm_add_ps(r12, r13)));

		auto r20 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(2), lhs.xmmd(2), _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmd(0));
		auto r21 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(2), lhs.xmmd(2), _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmd(1));
		auto r22 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(2), lhs.xmmd(2), _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmd(2));
		auto r23 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(2), lhs.xmmd(2), _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmd(3));
		auto r2 = _mm_add_ps(r20, _mm_add_ps(r21, _mm_add_ps(r22, r23)));
		
		auto r30 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(3), lhs.xmmd(3), _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmd(0));
		auto r31 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(3), lhs.xmmd(3), _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmd(1));
		auto r32 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(3), lhs.xmmd(3), _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmd(2));
		auto r33 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(3), lhs.xmmd(3), _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmd(3));
		auto r3 = _mm_add_ps(r30, _mm_add_ps(r31, _mm_add_ps(r32, r33)));

		return matrix4f(r0, r1, r2, r3);
#else
		matrix4f result;

		for (auto i = 0u; i != 4u; ++i)
		{
			for (auto j = 0u; j != 4u; ++j)
			{
				float r = 0.f;

				for (auto k = 0u; k != 4u; ++k)
					r += lhs.element(i, k) * rhs.element(k, j);

				result.set(i, j, r);
			}
		}

		return result;
#endif
	}

	inline auto operator * (matrix4f const& lhs, vector4f const& rhs) -> vector4f
	{
		__m128 rv0 = _mm_dp_ps(lhs.xmmd(0), rhs.xmmd(), 0xf1),
		       rv1 = _mm_dp_ps(lhs.xmmd(1), rhs.xmmd(), 0xf2),
			   rv2 = _mm_dp_ps(lhs.xmmd(2), rhs.xmmd(), 0xf4),
			   rv3 = _mm_dp_ps(lhs.xmmd(3), rhs.xmmd(), 0xf8)
			   ;

		auto k = _mm_add_ps(_mm_add_ps(rv0, rv1), _mm_add_ps(rv2, rv3));

		return vector4f(k);
	}
	
	inline auto operator * (matrix4f const& lhs, float f) -> matrix4f
	{
		__m128 ss = _mm_set_ps(f, f, f, f);

		__m128 r0 = _mm_mul_ps(lhs.xmmd(0), ss),
			   r1 = _mm_mul_ps(lhs.xmmd(1), ss),
			   r2 = _mm_mul_ps(lhs.xmmd(2), ss),
			   r3 = _mm_mul_ps(lhs.xmmd(3), ss)
			   ;
		
		return matrix4f(r0, r1, r2, r3);
	}

	inline auto operator + (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
		return matrix4f(
			_mm_add_ps(lhs.xmmd(0), rhs.xmmd(0)),
			_mm_add_ps(lhs.xmmd(1), rhs.xmmd(1)),
			_mm_add_ps(lhs.xmmd(2), rhs.xmmd(2)),
			_mm_add_ps(lhs.xmmd(3), rhs.xmmd(3))
		);
	}

	inline auto operator - (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
		return matrix4f(
			_mm_sub_ps(lhs.xmmd(0), rhs.xmmd(0)),
			_mm_sub_ps(lhs.xmmd(1), rhs.xmmd(1)),
			_mm_sub_ps(lhs.xmmd(2), rhs.xmmd(2)),
			_mm_sub_ps(lhs.xmmd(3), rhs.xmmd(3))
		);
	}

	inline auto transpose(matrix4f const& x) -> matrix4f
	{
		__m128 t0 = _mm_shuffle_ps(x.xmmd(0), x.xmmd(1), _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t1 = _mm_shuffle_ps(x.xmmd(0), x.xmmd(1), _MM_SHUFFLE(3, 2, 3, 2));
		__m128 t2 = _mm_shuffle_ps(x.xmmd(2), x.xmmd(3), _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t3 = _mm_shuffle_ps(x.xmmd(2), x.xmmd(3), _MM_SHUFFLE(3, 2, 3, 2));
		
		return matrix4f(
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(3, 1, 3, 1)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 1, 3, 1))
		);
	}

	inline auto inverse(matrix4f const& x) -> matrix4f
	{
		auto t = transpose(x);
		
		__m128 v00 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v02 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(0), _MM_SHUFFLE(2, 0, 2, 0));
		__m128 v10 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v11 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v12 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(1), _MM_SHUFFLE(3, 1, 3, 1));
		__m128 d0 = _mm_mul_ps(v00, v10);
		__m128 d1 = _mm_mul_ps(v01, v11);
		__m128 d2 = _mm_mul_ps(v02, v12);

		v00 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(3, 2, 3, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(3, 2, 3, 2));
		v02 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(0), _MM_SHUFFLE(3, 1, 3, 1));
		v10 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 1, 0, 0));
		v11 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(1, 1, 0, 0));
		v12 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(1), _MM_SHUFFLE(2, 0, 2, 0));
		d0 = _mm_sub_ps(d0, _mm_mul_ps(v00, v10));
		d1 = _mm_sub_ps(d1, _mm_mul_ps(v01, v11));
		d2 = _mm_sub_ps(d2, _mm_mul_ps(v02, v12));

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 1, 3, 1));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(0), _MM_SHUFFLE(1, 0, 2, 1));
		v10 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(0, 3, 0, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(0, 1, 0, 2));
		v11 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(2, 1, 2, 1));
		__m128 v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 3, 3, 1));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 0, 2, 1));
		v12 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(0, 3, 0, 2));
		__m128 v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(0, 1, 0, 2));
		v13 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(2, 1, 2, 1));

		__m128 c0 = _mm_mul_ps(v00, v10);
		__m128 c2 = _mm_mul_ps(v01, v11);
		__m128 c4 = _mm_mul_ps(v02, v12);
		__m128 c6 = _mm_mul_ps(v03, v13);

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(0, 0, 1, 0));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(2, 1, 3, 2));
		v10 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(1, 3, 2, 3));
		v11 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(0, 2, 1, 2));

		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 2, 1, 0));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(2, 1, 3, 2));
		v12 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(2, 1, 0, 3));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(1, 3, 2, 3));
		v13 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(0, 2, 1, 2));

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		c0 = _mm_sub_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_sub_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(0, 3, 0, 3));
		v10 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 2, 2));
		v10 = _mm_shuffle_ps(v10, v10, _MM_SHUFFLE(0, 2, 3, 0));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(2, 0, 3, 1));
		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 3, 0));
		v11 = _mm_shuffle_ps(v11, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(0, 3, 0, 3));
		v12 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 2, 2));
		v12 = _mm_shuffle_ps(v12, v12, _MM_SHUFFLE(0, 2, 3, 0));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(2, 0, 3, 1));
		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 3, 0));
		v13 = _mm_shuffle_ps(v13, v13, _MM_SHUFFLE(2, 1, 0, 3));

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		__m128 c1 = _mm_sub_ps(c0, v00);
		__m128 c3 = _mm_add_ps(c2, v01);
		__m128 c5 = _mm_sub_ps(c4, v02);
		__m128 c7 = _mm_add_ps(c6, v03);
		c0 = _mm_add_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_add_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

		c0 = _mm_shuffle_ps(c0, c1, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c3, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c5, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c7, _MM_SHUFFLE(3, 1, 2, 0));
		c0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c2, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c4, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c6, _MM_SHUFFLE(3, 1, 2, 0));

		// determinate
		__m128 det = _mm_dp_ps(c0, t.xmmd(0), 0xff);

		det = _mm_div_ps(xmmd_one_ps, det);

		return matrix4f(
			_mm_mul_ps(c0, det),
			_mm_mul_ps(c2, det),
			_mm_mul_ps(c4, det),
			_mm_mul_ps(c6, det)
		);
	}


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

		return transpose(result);
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
