//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4F_DECLARATION_HPP
#define ATMA_MATH_IMPL_VECTOR4F_DECLARATION_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4F_SCOPE
#	error "this file needs to be included solely from vector4f.hpp"
#endif
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	//=====================================================================
	// vector4f
	// ----------
	//   A four-component vector of floats.
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
		auto normalized() const -> impl::vector4f_div<vector4f, float>;

		
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
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
