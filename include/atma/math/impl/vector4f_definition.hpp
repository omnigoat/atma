//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4F_DEFINITION_HPP
#define ATMA_MATH_IMPL_VECTOR4F_DEFINITION_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4F_SCOPE
#	error "this file needs to be included solely from vector4f.hpp"
#endif
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	vector4f::vector4f()
	{
#if ATMA_MATH_USE_SSE
		xmmd_ = _mm_setzero_ps();
#else
		memset(fpd_, 0, 128);
#endif
	}

	vector4f::vector4f(float x, float y, float z, float w)
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_load_ps(&x);
#else
		memcpy(fpd_, &x, sizeof(float) * 4);
#endif
	}

#ifdef ATMA_MATH_USE_SSE
	vector4f::vector4f(__m128 xm)
		: xmmd_(xm)
	{
	}
#endif

	template <typename OP>
	vector4f::vector4f(impl::expr<vector4f, OP> const& expr)
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = expr.xmmd();
#else
#endif
	}

	template <typename OP>
	inline auto vector4f::operator = (const impl::expr<vector4f, OP>& e) -> vector4f&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = e.xmmd();
#else
		for (auto i = 0u; i != E; ++i) {
			fpd_[i] = e[i];
		}
#endif
		return *this;
	}


	inline auto vector4f::operator[](uint32_t i) const -> float
	{
		ATMA_ASSERT(i < 4);
#ifdef ATMA_MATH_USE_SSE
		return xmmd_.m128_f32[i];
#else
		return fpd_[i];
#endif
	}

	inline auto vector4f::magnitude() const -> float
	{
#ifdef ATMA_MATH_USE_SSE
		return _mm_sqrt_ss(_mm_dp_ps(xmmd_, xmmd_, 0x7f)).m128_f32[0];
#else
		return std::sqrt(magnitude_squared());
#endif
	}

	inline auto vector4f::magnitude_squared() const -> float
	{
#ifdef ATMA_MATH_USE_SSE
		return _mm_dp_ps(xmmd_, xmmd_, 0x7f).m128_f32[0];
#else
		return dot_product(*this, *this);
#endif
	}

	inline auto vector4f::normalized() const -> impl::vector4f_div<vector4f, float>
	{
		return { *this, magnitude_squared() };
	}

	template <typename OP>
	vector4f& vector4f::operator += (impl::expr<vector4f, OP> const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_add_ps(xmmd_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fpd_[i] += rhs.fpd_[i];
#endif
		return *this;
	}

	template <typename OP>
	vector4f& vector4f::operator -= (impl::expr<vector4f, OP> const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_sub_ps(xmmd_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fpd_[i] -= rhs.fpd_[i];
#endif
		return *this;
	}

	vector4f& vector4f::operator *= (float rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_sub_ps(xmmd_, _mm_load_ps1(&rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fpd_[i] *= rhs;
#endif
		return *this;
	}

	auto vector4f::operator /= (float rhs) -> vector4f&
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_sub_ps(xmmd_, _mm_load_ps1(&rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fpd_[i] /= rhs;
#endif
		return *this;
	}

	auto vector4f::set(uint32_t i, float n) -> void
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_.m128_f32[i] = n;
#else
		fpd_[i] = n;
#endif
	}

	auto vector4f::normalize() -> void
	{
#ifdef ATMA_MATH_USE_SSE
		xmmd_ = _mm_mul_ps(xmmd_, _mm_rsqrt_ps(_mm_dp_ps(xmmd_, xmmd_, 0x7f)));
#else
		*this /= magnitude();
#endif
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto dot_product(vector4f const& lhs, vector4f const& rhs) -> float
	{
#if ATMA_MATH_USE_SSE
		return _mm_dp_ps(lhs.xmmd(), rhs.xmmd(), 0x7f).m128_f32[0];
#else
		float result{};
		for (auto i = 0u; i != 4; ++i)
			result += lhs[i] * rhs[i];
		return result;
#endif
	}

	template <typename LOP, typename ROP>
	inline auto cross_product(impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs) -> vector4f
	{
#ifdef ATMA_MATH_USE_SSE
		return vector4f(
			_mm_sub_ps(
				_mm_mul_ps(
					_mm_shuffle_ps(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)),
					_mm_shuffle_ps(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(
					_mm_shuffle_ps(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2)),
					_mm_shuffle_ps(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)))));
#else
		return vector4f<3, float>{
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
