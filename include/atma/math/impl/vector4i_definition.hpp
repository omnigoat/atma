#pragma once

#ifndef ATMA_MATH_VECTOR4I_SCOPE
#	error "this file needs to be included solely from vector4i.hpp"
#endif

#include <atma/assert.hpp>


namespace atma { namespace math {

	inline vector4i::vector4i()
	{
		xmmdata = _mm_setzero_si128();
	}

	inline vector4i::vector4i(int32 x, int32 y, int32 z, int32 w)
	{
		__declspec(align(16)) int32 v[] = {x, y, z, w};
		xmmdata = _mm_load_si128((__m128i*)v);
	}

	vector4i::vector4i(__m128i xm)
		: xmmdata(xm)
	{
	}

	inline auto vector4i::operator = (vector4i const& e) -> vector4i&
	{
		xmmdata = e.xmmdata;
		return *this;
	}


	inline auto vector4i::operator *= (int32 rhs) -> vector4i&
	{
		xmmdata = _mm_mullo_epi32(xmmdata, _mm_set1_epi32(rhs));
		return *this;
	}

	inline auto vector4i::operator /= (int32 rhs) -> vector4i&
	{
		// lol no SSE for integer-divide
		for (auto i = 0u; i != 4u; ++i)
			components[i] /= rhs;
		return *this;
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto dot_product(vector4i const& lhs, vector4i const& rhs) -> int32
	{
		auto tmp = _mm_mullo_epi32(lhs.xmmdata, rhs.xmmdata);
		tmp = _mm_add_epi32(tmp, _mm_srli_si128(tmp, 8));
		tmp = _mm_add_epi32(tmp, _mm_srli_si128(tmp, 4));
		return _mm_cvtsi128_si32(tmp);
	}

#if 0
	template <typename LOP, typename ROP>
	inline auto cross_product(impl::expr<vector4i, LOP> const& lhs, impl::expr<vector4i, ROP> const& rhs) -> vector4i
	{
		return vector4i(
			_mm_sub_epi32(
				_mm_mul_epi32(
					_mm_shuffle_epi32(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(3, 0, 2, 1)),
					_mm_shuffle_epi32(rhs.xmmdata, rhs.xmmdata, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_epi32(
					_mm_shuffle_epi32(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_epi32(rhs.xmmdata, rhs.xmmdata, _MM_SHUFFLE(3, 0, 2, 1)))));
	}
#endif

} }


