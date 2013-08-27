//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_MATRIX_HPP
#define ATMA_MATH_MATRIX_HPP
//=====================================================================
#include <array>
#include <numeric>
#include <type_traits>
#include <initializer_list>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	__m128 const xmmd_one_f32x4 = _mm_set_ps(1.f, 1.f, 1.f, 1.f);


	struct matrix4f
	{
		static auto identity() -> matrix4f
		{
			static __m128 r0 = _mm_setr_ps(1.f, 0.f, 0.f, 0.f),
						  r1 = _mm_setr_ps(0.f, 1.f, 0.f, 0.f),
						  r2 = _mm_setr_ps(0.f, 0.f, 1.f, 0.f),
						  r3 = _mm_setr_ps(0.f, 0.f, 0.f, 1.f)
						  ;
			
			return matrix4f(r0, r1, r2, r3);
		}

		matrix4f()
		{
		}

		explicit matrix4f(__m128 const& r0, __m128 const& r1, __m128 const& r2, __m128 const& r3)
		{
			xmmd_[0] = r0;
			xmmd_[1] = r1;
			xmmd_[2] = r2;
			xmmd_[3] = r3;
		}

		auto set(uint32_t r, uint32_t c, float v) -> void
		{
			xmmd_[r].m128_f32[c] = v;
		}

		auto xmmd(uint32_t i) const -> __m128 const&
		{
			return xmmd_[i];
		}

	private:
#ifdef ATMA_MATH_USE_SSE
		__m128 xmmd_[4];
#else
		float fpd_[4][4];
#endif
	};
	

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

#if 0
		XMVECTOR V00 = XM_PERMUTE_PS(MT.r[2], _MM_SHUFFLE(1, 1, 0, 0));
		XMVECTOR V10 = XM_PERMUTE_PS(MT.r[3], _MM_SHUFFLE(3, 2, 3, 2));
		XMVECTOR V01 = XM_PERMUTE_PS(MT.r[0], _MM_SHUFFLE(1, 1, 0, 0));
		XMVECTOR V11 = XM_PERMUTE_PS(MT.r[1], _MM_SHUFFLE(3, 2, 3, 2));
		XMVECTOR V02 = _mm_shuffle_ps(MT.r[2], MT.r[0], _MM_SHUFFLE(2, 0, 2, 0));
		XMVECTOR V12 = _mm_shuffle_ps(MT.r[3], MT.r[1], _MM_SHUFFLE(3, 1, 3, 1));
#endif

		__m128 d0 = _mm_mul_ps(v00, v10);
		__m128 d1 = _mm_mul_ps(v01, v11);
		__m128 d2 = _mm_mul_ps(v02, v12);

#if 0
		XMVECTOR D0 = _mm_mul_ps(V00, V10);
		XMVECTOR D1 = _mm_mul_ps(V01, V11);
		XMVECTOR D2 = _mm_mul_ps(V02, V12);
#endif

		v00 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(3, 2, 3, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(3, 2, 3, 2));
		v02 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(0), _MM_SHUFFLE(3, 1, 3, 1));
		v10 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 1, 0, 0));
		v11 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(1, 1, 0, 0));
		v12 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(1), _MM_SHUFFLE(2, 0, 2, 0));
		
#if 0
		V00 = XM_PERMUTE_PS(MT.r[2], _MM_SHUFFLE(3, 2, 3, 2));
		V10 = XM_PERMUTE_PS(MT.r[3], _MM_SHUFFLE(1, 1, 0, 0));
		V01 = XM_PERMUTE_PS(MT.r[0], _MM_SHUFFLE(3, 2, 3, 2));
		V11 = XM_PERMUTE_PS(MT.r[1], _MM_SHUFFLE(1, 1, 0, 0));
		V02 = _mm_shuffle_ps(MT.r[2], MT.r[0], _MM_SHUFFLE(3, 1, 3, 1));
		V12 = _mm_shuffle_ps(MT.r[3], MT.r[1], _MM_SHUFFLE(2, 0, 2, 0));
#endif
		
		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		d0 = _mm_sub_ps(d0, v00);
		d1 = _mm_sub_ps(d1, v01);
		d2 = _mm_sub_ps(d2, v02);

#if 0
		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		D0 = _mm_sub_ps(D0, V00);
		D1 = _mm_sub_ps(D1, V01);
		D2 = _mm_sub_ps(D2, V02);
#endif


		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 1, 3, 1));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(0), _MM_SHUFFLE(1, 0, 2, 1));
		v10 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(0, 3, 0, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(0, 1, 0, 2));
		v11 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(2, 1, 2, 1));

		// v13 = d1Y,d1W,d2W,d2W
		__m128 v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 3, 3, 1));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 0, 2, 1));
		v12 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(0, 3, 0, 2));
		__m128 v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(0, 1, 0, 2));
		v13 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(2, 1, 2, 1));

#if 0
		// V11 = D0Y,D0W,D2Y,D2Y
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
		V00 = XM_PERMUTE_PS(MT.r[1], _MM_SHUFFLE(1, 0, 2, 1));
		V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
		V01 = XM_PERMUTE_PS(MT.r[0], _MM_SHUFFLE(0, 1, 0, 2));
		V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
		// V13 = D1Y,D1W,D2W,D2W
		XMVECTOR V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
		V02 = XM_PERMUTE_PS(MT.r[3], _MM_SHUFFLE(1, 0, 2, 1));
		V12 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
		XMVECTOR V03 = XM_PERMUTE_PS(MT.r[2], _MM_SHUFFLE(0, 1, 0, 2));
		V13 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));
#endif


		__m128 c0 = _mm_mul_ps(v00, v10);
		__m128 c2 = _mm_mul_ps(v01, v11);
		__m128 c4 = _mm_mul_ps(v02, v12);
		__m128 c6 = _mm_mul_ps(v03, v13);
		
#if 0
		XMVECTOR C0 = _mm_mul_ps(V00, V10);
		XMVECTOR C2 = _mm_mul_ps(V01, V11);
		XMVECTOR C4 = _mm_mul_ps(V02, V12);
		XMVECTOR C6 = _mm_mul_ps(V03, V13);
#endif
		// V11 = D0X,D0Y,D2X,D2X
		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(0, 0, 1, 0));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(2, 1, 3, 2));
		v10 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(1, 3, 2, 3));
		v11 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(0, 2, 1, 2));

		// V13 = D1X,D1Y,D2Z,D2Z
		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 2, 1, 0));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(2, 1, 3, 2));
		v12 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(2, 1, 0, 3));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(1, 3, 2, 3));
		v13 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(0, 2, 1, 2));

#if 0
		// V11 = D0X,D0Y,D2X,D2X
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
		V00 = XM_PERMUTE_PS(MT.r[1], _MM_SHUFFLE(2, 1, 3, 2));
		V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
		V01 = XM_PERMUTE_PS(MT.r[0], _MM_SHUFFLE(1, 3, 2, 3));
		V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
		// V13 = D1X,D1Y,D2Z,D2Z
		V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
		V02 = XM_PERMUTE_PS(MT.r[3], _MM_SHUFFLE(2, 1, 3, 2));
		V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
		V03 = XM_PERMUTE_PS(MT.r[2], _MM_SHUFFLE(1, 3, 2, 3));
		V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));
#endif

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		c0 = _mm_sub_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_sub_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

#if 0
		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		V03 = _mm_mul_ps(V03, V13);
		C0 = _mm_sub_ps(C0, V00);
		C2 = _mm_sub_ps(C2, V01);
		C4 = _mm_sub_ps(C4, V02);
		C6 = _mm_sub_ps(C6, V03);
#endif

		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(0, 3, 0, 3));
		// v10 = d0Z,d0Z,d2X,d2Y
		v10 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 2, 2));
		v10 = _mm_shuffle_ps(v10, v10, _MM_SHUFFLE(0, 2, 3, 0));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(2, 0, 3, 1));
		// v11 = d0X,d0W,d2X,d2Y
		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 3, 0));
		v11 = _mm_shuffle_ps(v11, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(0, 3, 0, 3));
		// v12 = d1Z,d1Z,d2Z,d2W
		v12 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 2, 2));
		v12 = _mm_shuffle_ps(v12, v12, _MM_SHUFFLE(0, 2, 3, 0));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(2, 0, 3, 1));
		// v13 = d1X,d1W,d2Z,d2W
		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 3, 0));
		v13 = _mm_shuffle_ps(v13, v13, _MM_SHUFFLE(2, 1, 0, 3));

#if 0
		V00 = XM_PERMUTE_PS(MT.r[1], _MM_SHUFFLE(0, 3, 0, 3));
		// V10 = D0Z,D0Z,D2X,D2Y
		V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
		V10 = XM_PERMUTE_PS(V10, _MM_SHUFFLE(0, 2, 3, 0));
		V01 = XM_PERMUTE_PS(MT.r[0], _MM_SHUFFLE(2, 0, 3, 1));
		// V11 = D0X,D0W,D2X,D2Y
		V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
		V11 = XM_PERMUTE_PS(V11, _MM_SHUFFLE(2, 1, 0, 3));
		V02 = XM_PERMUTE_PS(MT.r[3], _MM_SHUFFLE(0, 3, 0, 3));
		// V12 = D1Z,D1Z,D2Z,D2W
		V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
		V12 = XM_PERMUTE_PS(V12, _MM_SHUFFLE(0, 2, 3, 0));
		V03 = XM_PERMUTE_PS(MT.r[2], _MM_SHUFFLE(2, 0, 3, 1));
		// V13 = D1X,D1W,D2Z,D2W
		V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
		V13 = XM_PERMUTE_PS(V13, _MM_SHUFFLE(2, 1, 0, 3));
#endif

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
		
#if 0
		V00 = _mm_mul_ps(V00, V10);
		V01 = _mm_mul_ps(V01, V11);
		V02 = _mm_mul_ps(V02, V12);
		V03 = _mm_mul_ps(V03, V13);
		XMVECTOR C1 = _mm_sub_ps(C0, V00);
		C0 = _mm_add_ps(C0, V00);
		XMVECTOR C3 = _mm_add_ps(C2, V01);
		C2 = _mm_sub_ps(C2, V01);
		XMVECTOR C5 = _mm_sub_ps(C4, V02);
		C4 = _mm_add_ps(C4, V02);
		XMVECTOR C7 = _mm_add_ps(C6, V03);
		C6 = _mm_sub_ps(C6, V03);
#endif


		c0 = _mm_shuffle_ps(c0, c1, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c3, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c5, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c7, _MM_SHUFFLE(3, 1, 2, 0));
		c0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c2, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c4, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c6, _MM_SHUFFLE(3, 1, 2, 0));


#if 0
		C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
		C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
		C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
		C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
		C0 = XM_PERMUTE_PS(C0, _MM_SHUFFLE(3, 1, 2, 0));
		C2 = XM_PERMUTE_PS(C2, _MM_SHUFFLE(3, 1, 2, 0));
		C4 = XM_PERMUTE_PS(C4, _MM_SHUFFLE(3, 1, 2, 0));
		C6 = XM_PERMUTE_PS(C6, _MM_SHUFFLE(3, 1, 2, 0));
#endif

		// determinate
		__m128 det = _mm_dp_ps(c0, t.xmmd(0), 0xff);

		det = _mm_div_ps(xmmd_one_f32x4, det);

		return matrix4f(
			_mm_mul_ps(c0, det),
			_mm_mul_ps(c2, det),
			_mm_mul_ps(c4, det),
			_mm_mul_ps(c6, det)
		);
	}


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
