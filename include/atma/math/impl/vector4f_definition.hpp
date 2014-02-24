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
	
	inline vector4f::vector4f()
	{
#if ATMA_MATH_USE_SSE
		sd_ = _mm_setzero_ps();
#else
		memset(fd_, 0, 128);
#endif
	}

	inline vector4f::vector4f(float x, float y, float z, float w)
	{
#ifdef ATMA_MATH_USE_SSE
		__declspec(align(16)) float fs[] = {x, y, z, w};
		sd_ = _mm_load_ps(fs);
#else
		memcpy(fd_, &x, sizeof(float) * 4);
#endif
	}

#ifdef ATMA_MATH_USE_SSE
	inline vector4f::vector4f(__m128 xm)
		: sd_(xm)
	{
	}
#endif

	template <typename OP>
	inline vector4f::vector4f(impl::expr<vector4f, OP> const& expr)
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = expr.xmmd();
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] = &expr[i];
#endif
	}

	template <typename OP>
	inline auto vector4f::operator = (const impl::expr<vector4f, OP>& e) -> vector4f&
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = e.xmmd();
#else
		for (auto i = 0u; i != E; ++i) {
			fd_[i] = e[i];
		}
#endif
		return *this;
	}


	inline auto vector4f::operator[](uint32 i) const -> float
	{
		ATMA_ASSERT(i < 4);
#ifdef ATMA_MATH_USE_SSE
		return sd_.m128_f32[i];
#else
		return fd_[i];
#endif
	}

	inline auto vector4f::magnitude() const -> float
	{
#ifdef ATMA_MATH_USE_SSE
		return _mm_sqrt_ss(_mm_dp_ps(sd_, sd_, 0x7f)).m128_f32[0];
#else
		return std::sqrt(magnitude_squared());
#endif
	}

	inline auto vector4f::magnitude_squared() const -> float
	{
#ifdef ATMA_MATH_USE_SSE
		return _mm_dp_ps(sd_, sd_, 0x7f).m128_f32[0];
#else
		return dot_product(*this, *this);
#endif
	}

	inline auto vector4f::normalized() const -> impl::vector4f_div<vector4f, float>
	{
		return { *this, magnitude() };
	}

	template <typename OP>
	inline vector4f& vector4f::operator += (impl::expr<vector4f, OP> const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = _mm_add_ps(sd_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] += rhs.fd_[i];
#endif
		return *this;
	}

	template <typename OP>
	inline vector4f& vector4f::operator -= (impl::expr<vector4f, OP> const& rhs)
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = _mm_sub_ps(sd_, rhs.xmmd());
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] -= rhs.fd_[i];
#endif
		return *this;
	}

	inline auto vector4f::operator *= (float rhs) -> vector4f&
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = _mm_sub_ps(sd_, _mm_load_ps1(&rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] *= rhs;
#endif
		return *this;
	}

	inline auto vector4f::operator /= (float rhs) -> vector4f&
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = _mm_sub_ps(sd_, _mm_load_ps1(&rhs));
#else
		for (auto i = 0u; i != 4u; ++i)
			fd_[i] /= rhs;
#endif
		return *this;
	}

	inline auto vector4f::set(uint32 i, float n) -> void
	{
#ifdef ATMA_MATH_USE_SSE
		sd_.m128_f32[i] = n;
#else
		fd_[i] = n;
#endif
	}

	inline auto vector4f::normalize() -> void
	{
#ifdef ATMA_MATH_USE_SSE
		sd_ = _mm_mul_ps(sd_, _mm_rsqrt_ps(_mm_dp_ps(sd_, sd_, 0x7f)));
#else
		*this /= magnitude();
#endif
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto point4f(float x, float y, float z) -> vector4f
	{
		return vector4f(x, y, z, 1.f);
	}

	inline auto point4f() -> vector4f
	{
		return vector4f(0.f, 0.f, 0.f, 1.f);
	}

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
