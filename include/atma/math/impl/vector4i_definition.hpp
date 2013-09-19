//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4I_DEFINITION_HPP
#define ATMA_MATH_IMPL_VECTOR4I_DEFINITION_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4I_SCOPE
#	error "this file needs to be included solely from vector4i.hpp"
#endif
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	vector4i::vector4i()
	{
#if ATMA_MATH_USE_SSE
		//sse_ = _mm_setzero_epi32();
#else
		memset(fd_, 0, 128);
#endif
	}

	vector4i::vector4i(int32_t x, int32_t y, int32_t z, int32_t w)
	{
#ifdef ATMA_MATH_USE_SSE
		__declspec(align(16)) int32_t fs[] = {x, y, z, w};
		sse_ = _mm_load_si32((__m128i*)fs);
#else
		memcpy(fd_, &x, sizeof(int32_t) * 4);
#endif
	}

#ifdef ATMA_MATH_USE_SSE
	vector4i::vector4i(__m128i xm)
		: sse_(xm)
	{
	}
#endif


	template <typename OP>
	inline auto vector4i::operator = (vector4i const& e) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		sse_ = e.xmmd();
#else
		for (auto i = 0u; i != E; ++i) {
			fd_[i] = e[i];
		}
#endif
		return *this;
	}


	inline auto vector4i::operator[](uint32_t i) const -> int32_t
	{
		ATMA_ASSERT(i < 4);
#ifdef ATMA_MATH_USE_SSE
		return sse_.m128i_i32[i];
#else
		return fd_[i];
#endif
	}
	

	template <typename OP>
	vector4i& vector4i::operator += (vector4i const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		sse_ = _mm_add_epi32(sse_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] += rhs.fd_[i];
#endif
		return *this;
	}

	template <typename OP>
	vector4i& vector4i::operator -= (vector4i const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		sse_ = _mm_sub_epi32(sse_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] -= rhs.fd_[i];
#endif
		return *this;
	}

	vector4i& vector4i::operator *= (int32_t rhs)
	{
#ifdef ATMA_MATH_USE_SSE
#	ifdef ATMA_MATH_SSE_4_1
		sse_ = _mm_mullo_epi32(sse_, _mm_set1_epi32(rhs))
#	else
		sse_ = _mm_sub_epi32(sse_, _mm_load_epi321(&rhs));
#	endif
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] *= rhs;
#endif
		return *this;
	}

	auto vector4i::operator /= (int32_t rhs) -> vector4i&
	{
#ifdef ATMA_MATH_USE_SSE
		sse_ = _mm_sub_epi32(sse_, _mm_load_epi321(&rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] /= rhs;
#endif
		return *this;
	}

	auto vector4i::set(uint32_t i, int32_t n) -> void
	{
#ifdef ATMA_MATH_USE_SSE
		sse_.m128_f32[i] = n;
#else
		fd_[i] = n;
#endif
	}

	auto vector4i::normalize() -> void
	{
#ifdef ATMA_MATH_USE_SSE
		sse_ = _mm_mul_epi32(sse_, _mm_rsqrt_epi32(_mm_dp_epi32(sse_, sse_, 0x7f)));
#else
		*this /= magnitude();
#endif
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto dot_product(vector4i const& lhs, vector4i const& rhs) -> int32_t
	{
#if ATMA_MATH_USE_SSE
		return _mm_dp_epi32(lhs.xmmd(), rhs.xmmd(), 0x7f).m128_f32[0];
#else
		int32_t result{};
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
		return vector4i<3, int32_t>{
			lhs[1]*rhs[2] - lhs[2]*rhs[1],
			lhs[2]*rhs[0] - lhs[0]*rhs[2],
			lhs[0]*rhs[1] - lhs[1]*rhs[0]
		};
#endif
	}

//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
