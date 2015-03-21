#pragma once

#include <atma/types.hpp>

#pragma warning(push,3)
#include <smmintrin.h>
#pragma warning(pop)


namespace atma { namespace math {

	__declspec(align(16))
	struct vector4f
	{
		vector4f();
		explicit vector4f(__m128);
		vector4f(float x, float y, float z);
		vector4f(float x, float y, float z, float w);
		vector4f(vector4f const&) = default;

		auto operator = (vector4f const&) -> vector4f&;
		
		auto is_zero() const -> bool;
		auto magnitude_squared() const -> float;
		auto magnitude() const -> float;

		// mutation
		auto operator += (vector4f const&) -> vector4f&;
		auto operator -= (vector4f const&) -> vector4f&;
		auto operator *= (float) -> vector4f&;
		auto operator /= (float) -> vector4f&;

#pragma warning(push)
#pragma warning(disable: 4201)
		union
		{
			__m128 xmmdata;

			float components[4];

			struct {
				float x, y, z, w;
			};
		};
#pragma warning(pop)
	};




	//=====================================================================
	// implementation
	//=====================================================================
	inline vector4f::vector4f()
		: xmmdata(_mm_setzero_ps())
	{
	}

	inline vector4f::vector4f(float x, float y, float z, float w)
	{
		xmmdata = _mm_set_ps(w, z, y, x);
	}

	inline vector4f::vector4f(float x, float y, float z)
	{
		xmmdata = _mm_set_ps(0.f, z, y, x);
	}

	inline vector4f::vector4f(__m128 xm)
		: xmmdata(xm)
	{
	}

	inline auto vector4f::operator = (vector4f const& rhs) -> vector4f&
	{
		xmmdata = rhs.xmmdata;
		return *this;
	}

	inline auto vector4f::magnitude() const -> float
	{
		return _mm_sqrt_ss(_mm_dp_ps(xmmdata, xmmdata, 0x7f)).m128_f32[0];
	}

	inline auto vector4f::magnitude_squared() const -> float
	{
		return _mm_dp_ps(xmmdata, xmmdata, 0x7f).m128_f32[0];
	}

	inline auto vector4f::operator += (vector4f const& rhs) -> vector4f&
	{
		xmmdata = _mm_add_ps(xmmdata, rhs.xmmdata);
		return *this;
	}

	inline auto vector4f::operator -= (vector4f const& rhs) -> vector4f&
	{
		xmmdata = _mm_sub_ps(xmmdata, rhs.xmmdata);
		return *this;
	}

	inline auto vector4f::operator *= (float rhs) -> vector4f&
	{
		xmmdata = _mm_mul_ps(xmmdata, _mm_set_ps(rhs, rhs, rhs, rhs));
		return *this;
	}

	inline auto vector4f::operator /= (float rhs) -> vector4f&
	{
		xmmdata = _mm_div_ps(xmmdata, _mm_set_ps(rhs, rhs, rhs, rhs));
		return *this;
	}




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator + (vector4f const& lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f{_mm_add_ps(lhs.xmmdata, rhs.xmmdata)};
	}


	inline auto operator - (vector4f const& lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f{_mm_sub_ps(lhs.xmmdata, rhs.xmmdata)};
	}


	inline auto operator * (vector4f const& lhs, float rhs) -> vector4f
	{
		return vector4f{_mm_mul_ps(lhs.xmmdata, _mm_set_ps(rhs, rhs, rhs, rhs))};
	}

	inline auto operator * (float lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f{_mm_mul_ps(_mm_set_ps(lhs, lhs, lhs, lhs), rhs.xmmdata)};
	}


	inline auto operator / (vector4f const& lhs, float rhs) -> vector4f
	{
		return vector4f{_mm_div_ps(lhs.xmmdata, _mm_set_ps(rhs, rhs, rhs, rhs))};
	}

	inline auto operator / (float lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f{_mm_div_ps(_mm_set_ps(lhs, lhs, lhs, lhs), rhs.xmmdata)};
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto point4f() -> vector4f
	{
		return vector4f{0.f, 0.f, 0.f, 1.f};
	}

	inline auto point4f(float x, float y, float z) -> vector4f
	{
		return vector4f{x, y, z, 1.f};
	}

	inline auto normalize(vector4f const& x) -> vector4f
	{
		return vector4f{_mm_mul_ps(x.xmmdata, _mm_rsqrt_ps(_mm_dp_ps(x.xmmdata, x.xmmdata, 0x7f)))};
	}

	inline auto dot_product(vector4f const& lhs, vector4f const& rhs) -> float
	{
		return _mm_dp_ps(lhs.xmmdata, rhs.xmmdata, 0x7f).m128_f32[0];
	}

	inline auto cross_product(vector4f const& lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f(
			_mm_sub_ps(
				_mm_mul_ps(
					_mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(3, 0, 2, 1)),
					_mm_shuffle_ps(rhs.xmmdata, rhs.xmmdata, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(
					_mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_ps(rhs.xmmdata, rhs.xmmdata, _MM_SHUFFLE(3, 0, 2, 1)))));
	}

} }

namespace aml = atma::math;