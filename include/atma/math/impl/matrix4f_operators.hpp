//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_MATRIX4F_OPERATORS_HPP
#define ATMA_MATH_IMPL_MATRIX4F_OPERATORS_HPP
//=====================================================================
#include <atma/math/impl/constants.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	

	//=====================================================================
	// matrix -> matrix -> matrix
	//=====================================================================
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

	


	//=====================================================================
	// matrix -> vector -> vector
	//=====================================================================
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
	



	//=====================================================================
	// matrix -> scalar -> matrix
	//=====================================================================
	inline auto operator * (matrix4f const& lhs, float f) -> matrix4f
	{
		__m128 ss = _am_load_f32x4(f, f, f, f);

		__m128 r0 = _mm_mul_ps(lhs.xmmd(0), ss),
		       r1 = _mm_mul_ps(lhs.xmmd(1), ss),
		       r2 = _mm_mul_ps(lhs.xmmd(2), ss),
		       r3 = _mm_mul_ps(lhs.xmmd(3), ss)
		       ;
		
		return matrix4f(r0, r1, r2, r3);
	}

	inline auto operator / (matrix4f const& lhs, float f) -> matrix4f
	{
		__m128 ss = _am_load_f32x4(f, f, f, f);

		__m128 r0 = _mm_div_ps(lhs.xmmd(0), ss),
			   r1 = _mm_div_ps(lhs.xmmd(1), ss),
			   r2 = _mm_div_ps(lhs.xmmd(2), ss),
			   r3 = _mm_div_ps(lhs.xmmd(3), ss)
			   ;

		return matrix4f(r0, r1, r2, r3);
	}


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
