//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_VECTOR_HPP
#define ATMA_MATH_VECTOR_HPP
//=====================================================================
#include <array>
#include <numeric>
#include <type_traits>
#include <initializer_list>
#include <atma/assert.hpp>
#include <atma/math/impl/expr.hpp>
#include <atma/math/impl/operators.hpp>
#include <atma/math/impl/binary_operator.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	//=====================================================================
	// forward-declare
	//=====================================================================
	struct vector4f;
	namespace impl {
		template <typename, typename> struct vector4f_add;
		template <typename, typename> struct vector4f_sub;
		template <typename, typename> struct vector4f_mul;
		template <typename, typename> struct vector4f_div;
	}



	//=====================================================================
	// vector4f
	// ---------
	//=====================================================================
	struct vector4f : impl::expr<vector4f, vector4f>
	{
		// construction
		vector4f();
		vector4f(float x, float y, float z, float w);
		explicit vector4f(__m128);
		template <typename OP>
		vector4f(impl::expr<vector4f, OP> const&);

		// assignment
		template <class OP>
		auto operator = (const impl::expr<vector4f, OP>& e) -> vector4f&;

		// access
#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 { return xmmd_; }
#endif
		auto operator[] (uint32_t i) const -> float;
		
		// computation
		auto is_zero() const -> bool;
		auto magnitude_squared() const -> float;
		auto magnitude() const -> float;

		//auto normalized() const -> impl::vector4f_div<vector4f, float>;

		
		// mutation
		template <typename OP> vector4f& operator += (impl::expr<vector4f, OP> const&);
		template <typename OP> vector4f& operator -= (impl::expr<vector4f, OP> const&);
		vector4f& operator *= (float rhs);
		vector4f& operator /= (float rhs);
		auto set(uint32_t i, float n) -> void;
		auto normalize() -> void;

	private:
#ifdef ATMA_MATH_USE_SSE
		__m128 xmmd_;
#else
		float fpd_[4];
#endif
	};
	

	//=====================================================================
	// functions
	//=====================================================================
	inline auto dot_product(vector4f const& lhs, vector4f const& rhs) -> float;
	inline auto cross_product(vector4f const& lhs, vector4f const& rhs) -> vector4f;





	//=====================================================================
	//
	//  IMPLEMENTATION
	//
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
		xmmd_ = _mm_load_ps(&x);
	}

	vector4f::vector4f(__m128 xm)
		: xmmd_(xm)
	{
	}

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

	//inline auto vector4f::normalized() const -> impl::vector4f_div<vector4f, float>
	//{
	//	return { *this, magnitude_squared() };
	//}

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
		return vector4f(_mm_sub_ps(
			_mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2))),
			_mm_mul_ps(_mm_shuffle_ps(lhs.xmmd(), lhs.xmmd(), _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(rhs.xmmd(), rhs.xmmd(), _MM_SHUFFLE(3, 0, 2, 1)))
		));
#else
		return vector4f<3, float>{
			lhs[1]*rhs[2] - lhs[2]*rhs[1],
			lhs[2]*rhs[0] - lhs[0]*rhs[2],
			lhs[0]*rhs[1] - lhs[1]*rhs[0]
		};
#endif
	}





	//=====================================================================
	//
	//  EXPRESSION TEMPLATES
	//
	//=====================================================================
	namespace impl
	{
#ifdef ATMA_MATH_USE_SSE
		template <typename OP>
		auto xmmd_of(expr<vector4f, OP> const& expr) -> __m128
		{
			return expr.xmmd();
		}

		auto xmmd_of(float x) -> __m128
		{
			return _mm_load_ps1(&x);
		}

#else
		template <typename T>
		auto element_of(T const& x, uint32_t i) -> float
		{
			return x[i];
		}

		auto element_of(float x, uint32_t) -> float
		{
			return x;
		}
#endif
		
		// binary_expr
		template <typename R, template<typename,typename> class OPER, typename LHS, typename RHS>
		struct binary_expr : expr<R, OPER<LHS, RHS>>
		{
			binary_expr(LHS const& lhs, RHS const& rhs)
				: lhs(lhs), rhs(rhs)
			{}

			typename storage_policy<typename std::decay<LHS>::type>::type lhs;
			typename storage_policy<typename std::decay<RHS>::type>::type rhs;
		};


		// vector4f_add
		template <typename LHS, typename RHS>
		struct vector4f_add : binary_expr<vector4f, vector4f_add, LHS, RHS>
		{
			vector4f_add(LHS const& lhs, RHS const& rhs)
			: binary_expr(lhs, rhs)
			{}

#ifdef ATMA_MATH_USE_SSE
			auto xmmd() const -> __m128 { return _mm_add_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
			auto element(uint32_t i) const -> float { return element_of(lhs, i) + element_of(rhs, i); }
#endif
		};

		// vector4f_sub
		template <typename LHS, typename RHS>
		struct vector4f_sub : binary_expr<vector4f, vector4f_sub, LHS, RHS>
		{
			vector4f_sub(LHS const& lhs, RHS const& rhs)
			: binary_expr(lhs, rhs)
			{
			}

#ifdef ATMA_MATH_USE_SSE
			auto xmmd() const -> __m128 { return _mm_sub_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
			auto element(uint32_t i) const -> float { return element_of(lhs, i) - element_of(rhs, i); }
#endif
		};

		// vector4f_mul
		template <typename LHS, typename RHS>
		struct vector4f_mul : binary_expr<vector4f, vector4f_mul, LHS, RHS>
		{
			vector4f_mul(LHS const& lhs, RHS const& rhs)
			: binary_expr(lhs, rhs)
			{
			}

#ifdef ATMA_MATH_USE_SSE
			auto xmmd() const -> __m128 { return _mm_mul_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
			auto element(uint32_t i) const -> float { return element_of(lhs, i) * element_of(rhs, i); }
#endif
		};

		// vector4f_div
		template <typename LHS, typename RHS>
		struct vector4f_div : binary_expr<vector4f, vector4f_div, LHS, RHS>
		{
			vector4f_div(LHS const& lhs, RHS const& rhs)
			: binary_expr(lhs, rhs)
			{
			}

#ifdef ATMA_MATH_USE_SSE
			auto xmmd() const -> __m128 { return _mm_div_ps(xmmd_of(lhs), xmmd_of(rhs)); }
#else
			auto element(uint32_t i) const -> float { return element_of(lhs, i) / element_of(rhs, i); }
#endif
		};
	}


	//=====================================================================
	// addition
	//=====================================================================
	// vector + vector
	inline auto operator + (vector4f const& lhs, vector4f const& rhs)
		-> impl::vector4f_add<vector4f, vector4f>
	{
		return { lhs, rhs };
	}

	// expr + vector
	template <typename OP>
	inline auto operator + (impl::expr<vector4f, OP> const& lhs, vector4f const& rhs)
		-> impl::vector4f_add<impl::expr<vector4f, OP>, vector4f>
	{
		return { lhs, rhs };
	}

	// vector + expr
	template <typename OP>
	inline auto operator + (vector4f const& lhs, impl::expr<vector4f, OP> const& rhs)
		-> impl::vector4f_add<vector4f, impl::expr<vector4f, OP>>
	{
		return { lhs, rhs };
	}

	// expr + expr
	template <typename LOP, typename ROP>
	inline auto operator + (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
		-> impl::vector4f_add<impl::expr<vector4f, LOP>, impl::expr<vector4f, ROP>>
	{
		return { lhs, rhs };
	}



	//=====================================================================
	// subtraction
	//=====================================================================
	// vector - vector
	inline auto operator - (vector4f const& lhs, vector4f const& rhs)
		-> impl::vector4f_sub<vector4f, vector4f>
	{
		return { lhs, rhs };
	}

	// expr - vector
	template <typename OP>
	inline auto operator - (impl::expr<vector4f, OP> const& lhs, vector4f const& rhs)
		-> impl::vector4f_sub<impl::expr<vector4f, OP>, vector4f>
	{
		return { lhs, rhs };
	}

	// vector - expr
	template <typename OP>
	inline auto operator - (vector4f const& lhs, impl::expr<vector4f, OP> const& rhs)
		-> impl::vector4f_sub<vector4f, impl::expr<vector4f, OP>>
	{
		return { lhs, rhs };
	}

	// expr - expr
	template <typename LOP, typename ROP>
	inline auto operator - (impl::expr<vector4f, LOP> const& lhs, impl::expr<vector4f, ROP> const& rhs)
		-> impl::vector4f_sub<impl::expr<vector4f, LOP>, impl::expr<vector4f, ROP>>
	{
		return { lhs, rhs };
	}


	//=====================================================================
	// multiplication
	//    note: we don't have an expr for the scalar, because any expression
	//          that results in a single element almost definitely has high
	//          computational costs (like dot-products)
	//=====================================================================
	// vector * float
	inline auto operator * (vector4f const& lhs, float rhs)
		-> impl::vector4f_mul<vector4f, float>
	{
		return { lhs, rhs };
	}

	// X * float
	template <typename OPER>
	inline auto operator * (impl::expr<vector4f, OPER> const& lhs, float rhs)
		-> impl::vector4f_mul<impl::expr<vector4f, OPER>, float>
	{
		return { lhs, rhs };
	}

	// float * vector
	inline auto operator * (float lhs, vector4f const& rhs)
		-> impl::vector4f_mul<float, vector4f>
	{
		return { lhs, rhs };
	}

	// float * X
	template <typename OPER>
	inline auto operator * (float lhs, impl::expr<vector4f, OPER> const& rhs)
		-> impl::vector4f_mul<float, impl::expr<vector4f, OPER>>
	{
		return { lhs, rhs };
	}

	

	//=====================================================================
	// division
	//=====================================================================
	// vector / float
	inline auto operator / (vector4f const& lhs, float rhs)
		-> impl::vector4f_div<vector4f, float>
	{
		return { lhs, rhs };
	}

	// X / float
	template <typename OPER>
	inline auto operator / (impl::expr<vector4f, OPER> const& lhs, float rhs)
		-> impl::vector4f_div<impl::expr<vector4f, OPER>, float>
	{
		return { lhs, rhs };
	}


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
