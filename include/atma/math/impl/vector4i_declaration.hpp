//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_VECTOR4I_DECLARATION_HPP
#define ATMA_MATH_IMPL_VECTOR4I_DECLARATION_HPP
//=====================================================================
#ifndef ATMA_MATH_VECTOR4I_SCOPE
#	error "this file needs to be included solely from vector4i.hpp"
#endif
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	//=====================================================================
	// vector4i
	// ----------
	//   A four-component vector of int32_ts, 16-byte aligned
	//=====================================================================
	__declspec(align(16))
	struct vector4i
	{
		// construction
		vector4i();
		vector4i(int32_t x, int32_t y, int32_t z, int32_t w);
		explicit vector4i(__m128i);
		
		// assignment
		auto operator = (vector4i const& e) -> vector4i&;

		// access
#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128i { return sse_; }
#endif
		auto operator[] (uint32 i) const -> int32_t;
		
		// computation
		auto is_zero() const -> bool;
				
		// mutation
		template <typename OP> vector4i& operator += (impl::expr<vector4i, OP> const&);
		template <typename OP> vector4i& operator -= (impl::expr<vector4i, OP> const&);
		vector4i& operator *= (int32_t rhs);
		vector4i& operator /= (int32_t rhs);
		auto normalize() -> void;

		template <uint8_t i>
		struct elem_ref_t
		{
			elem_ref_t(vector4i* owner) : owner_(owner) {}

			operator int32_t() {
				return owner_->sse_.m128i_i32[i];
			}

			int32_t operator = (int32_t rhs) {
				owner_->sse_.m128i_i32[i] = rhs;
				return rhs;
			}

		private:
			vector4i* owner_;
		};
		
	private:
#ifdef ATMA_MATH_USE_SSE
		__m128i sse_;
#else
		int32_t x, y, z, w;
#endif
	};
	


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
