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
	//   A four-component vector of floats, 16-byte aligned
	//=====================================================================
	__declspec(align(16))
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
		auto xmmd() const -> __m128 { return sd_; }
#endif
		auto operator[] (uint32 i) const -> float;
		
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
		auto set(uint32 i, float n) -> void;
		auto normalize() -> void;

		template <uint8_t i>
		struct elem_ref_t
		{
			elem_ref_t(vector4f* owner) : owner_(owner) {}

			operator float() {
				return owner_->sd_.m128_f32[i];
			}

			float operator = (float rhs) {
				owner_->sd_.m128_f32[i] = rhs;
				return rhs;
			}

		private:
			vector4f* owner_;
		};
		
	private:
#ifdef ATMA_MATH_USE_SSE
#pragma warning(push)
#pragma warning(disable: 4201)
		union {
			__m128 sd_;
			struct { float x, y, z, w; };
		};
#pragma warning(pop)
#else
		float x, y, z, w;
#endif
	};
	

	//=====================================================================
	// functions
	//=====================================================================

	// returns a vector4f with the w-component set to 1.f
	inline auto point4f(float x, float y, float z) -> vector4f;
	inline auto point4f() -> vector4f;

	inline auto dot_product(vector4f const& lhs, vector4f const& rhs) -> float;

	template <typename LOP, typename ROP>
	inline auto cross_product(impl::expr<vector4f, LOP> const&, impl::expr<vector4f, ROP> const&) -> vector4f;


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
