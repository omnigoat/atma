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
	// forward-declare expression template functions
	//=====================================================================
	namespace impl
	{
		struct vector4f_add;
		struct vector4f_sub;
		struct vector4f_mul_post;
		struct vector4f_mul_pre;
		struct vector4f_div;
	}



	//=====================================================================
	// vector4f
	// ---------
	//=====================================================================
	struct vector4f
	{
		// construction
		vector4f();
		vector4f(float x, float y, float z, float w);
		vector4f(vector4f const& rhs);
		
		// assignment
		auto operator = (vector4f const& rhs) -> vector4f&;
		template <class OP>
		auto operator = (const impl::expr<vector4f, OP>& e) -> vector4f&;

		// access
		auto operator[] (uint32_t i) const -> float;
		
		// computation
		auto magnitude_squared() const -> float;
		auto magnitude() const -> float;
		//auto normalized() const -> impl::expr<vector4f, impl::binary_oper<impl::vector_div<E,float>, vector4f, float>>;


		// mutation
		vector4f& operator += (vector4f const& rhs);
		vector4f& operator -= (vector4f const& rhs);
		vector4f& operator *= (float rhs);
		vector4f& operator /= (float rhs);
		vector4f& set(uint32_t i, float n);
		vector4f& normalize();

	private:
#ifdef ATMA_MATH_USE_SSE
		union
		{
			// floating-point-register data
			struct { float fpd_[4]; };
			// xmm-register data
			__m128 xmmd_;
		};
#else
		float fpd_[4];
#endif

		template <typename R, typename OPER>
		friend struct impl::expr;
	};
	

	//=====================================================================
	// dot-product
	//=====================================================================
	inline auto dot_product(vector4f const& lhs, vector4f const& rhs) -> float
	{
		float result{};
		for (auto i = 0u; i != 4; ++i)
			result += lhs[i] * rhs[i];
		return result;
	}

	//=====================================================================
	// cross-product (3-dimensional only)
	//=====================================================================
	#if 0
	inline auto cross_product(vector4f const& lhs, vector4f const& rhs) -> vector4f
	{
		return vector4f<3,float>{
			lhs[1]*rhs[2] - lhs[2]*rhs[1],
			lhs[2]*rhs[0] - lhs[0]*rhs[2],
			lhs[0]*rhs[1] - lhs[1]*rhs[0]
		};
	}
	#endif
	

	// implementation
	#include "impl/vector_impl.hpp"
	// operators
	#include "impl/vector_opers.hpp"

//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
