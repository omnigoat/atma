#pragma once

#ifndef ATMA_MATH_VECTOR4I_SCOPE
#	error "this file needs to be included solely from vector4i.hpp"
#endif

#include <atma/assert.hpp>


namespace atma { namespace math {

	inline vector4i::vector4i()
	{
#if ATMA_MATH_USE_SSE
		xmmdata = _mm_setzero_si128();
#else
		memset(fd_, 0, 128);
#endif
	}

	inline vector4i::vector4i(int32 x, int32 y, int32 z, int32 w)
	{
#ifdef ATMA_MATH_USE_SSE
		__declspec(align(16)) int32 v[] = {x, y, z, w};
		xmmdata = _mm_load_si128((__m128i*)v);
#else
		memcpy(fd_, &x, sizeof(int32) * 4);
#endif
	}

#ifdef ATMA_MATH_USE_SSE
	vector4i::vector4i(__m128i xm)
		: xmmdata(xm)
	{
	}
#endif

	template <typename OP>
	inline auto vector4i::operator = (impl::expr<vector4i, OP> const& e) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmdata = e.xmmd();
#else
		for (auto i = 0u; i != E; ++i) {
			fd_[i] = e[i];
		}
#endif
		return *this;
	}


	inline auto vector4i::operator[](int i) const -> int32
	{
		ATMA_ASSERT(0 <= i && i < 4);
#ifdef ATMA_MATH_USE_SSE
		return xmmdata.m128i_i32[i];
#else
		return fd_[i];
#endif
	}
	

	template <typename OP>
	inline auto vector4i::operator += (impl::expr<vector4i, OP> const& rhs) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmdata = _mm_add_epi32(xmmdata, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] += rhs.fd_[i];
#endif
		return *this;
	}

	template <typename OP>
	inline auto vector4i::operator -= (impl::expr<vector4i, OP> const& rhs) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmdata = _mm_sub_epi32(xmmdata, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] -= rhs.fd_[i];
#endif
		return *this;
	}

	inline auto vector4i::operator *= (int32 rhs) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
#	ifdef ATMA_MATH_SSE_4_1
		xmmdata = _mm_mullo_epi32(xmmdata, _mm_set1_epi32(rhs))
#	else
		xmmdata = _mm_sub_epi32(xmmdata, _mm_set1_epi32(rhs));
#	endif
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] *= rhs;
#endif
		return *this;
	}

	inline auto vector4i::operator /= (int32 rhs) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmdata = _mm_sub_epi32(xmmdata, _mm_set1_epi32(rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] /= rhs;
#endif
		return *this;
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto dot_product(vector4i const& lhs, vector4i const& rhs) -> int32
	{
#if ATMA_MATH_USE_SSE
		auto tmp = _mm_mullo_epi32(lhs.xmmd(), rhs.xmmd());
		tmp = _mm_add_epi32(tmp, _mm_srli_si128(tmp, 8));
		tmp = _mm_add_epi32(tmp, _mm_srli_si128(tmp, 4));
		return _mm_cvtsi128_si32(tmp);
#else
		int32 result{};
		for (auto i = 0u; i != 4; ++i)
			result += lhs[i] * rhs[i];
		return result;
#endif
	}

	template <typename LOP, typename ROP>
	inline auto cross_product(impl::expr<vector4i, LOP> const& lhs, impl::expr<vector4i, ROP> const& rhs) -> vector4i
	{
#ifdef ATMA_MATH_USE_SSE
		return vector4i(
			_mm_sub_epi32(
				_mm_mul_epi32(
					_mm_shuffle_epi32(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)),
					_mm_shuffle_epi32(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_epi32(
					_mm_shuffle_epi32(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_epi32(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)))));
#else
		return vector4i<3, int32>{
			lhs[1]*rhs[2] - lhs[2]*rhs[1],
			lhs[2]*rhs[0] - lhs[0]*rhs[2],
			lhs[0]*rhs[1] - lhs[1]*rhs[0]
		};
#endif
	}


} }


