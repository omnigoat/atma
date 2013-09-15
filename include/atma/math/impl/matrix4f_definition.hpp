//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_MATRIX4F_DEFINITION_HPP
#define ATMA_MATH_IMPL_MATRIX4F_DEFINITION_HPP
//=====================================================================
#include <atma/math/impl/constants.hpp>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	

	//=====================================================================
	// impl
	//=====================================================================
	namespace impl
	{
		// cell-element-ref
		template <typename T>
		cell_element_ref<T>::cell_element_ref(matrix4f* owner, uint32_t row, uint32_t col)
		: owner_(owner), row_(row), col_(col)
		{}

		template <typename T>
		auto cell_element_ref<T>::operator = (float rhs) -> float
		{
#ifdef ATMA_MATH_USE_SSE
			return owner_->sd_[row_].m128_f32[col_] = rhs;
#else
			return owner_->fd_[row_][col_] = rhs;
#endif
		}

		template <typename T>
		cell_element_ref<T>::operator float() {
#ifdef ATMA_MATH_USE_SSE
			return owner_->sd_[row_].m128_f32[col_];
#else
			return owner_->fd_[row_][col_];
#endif
		}

		// cell-element-ref (const)
		template <typename T>
		cell_element_ref<T const>::cell_element_ref(matrix4f const* owner, uint32_t row, uint32_t col)
		: owner_(owner), row_(row), col_(col)
		{}

		template <typename T>
		cell_element_ref<T const>::operator float() {
#ifdef ATMA_MATH_USE_SSE
			return owner_->sd_[row_].m128_f32[col_];
#else
			return owner_->fd_[row_][col_];
#endif
		}


		// row-element-ref
		template <typename T>
		row_element_ref<T>::row_element_ref(matrix4f* owner, uint32_t row)
		: owner_(owner), row_(row)
		{}

		template <typename T>
		auto row_element_ref<T>::operator[](uint32_t i) -> cell_element_ref<T>
		{
			return cell_element_ref<T>(owner_, row_, i);
		}

		template <typename T>
		auto row_element_ref<T>::operator[](uint32_t i) const -> cell_element_ref<T const>
		{
			return cell_element_ref<T const>(owner_, row_, i);
		}

		// row-element-ref (const)
		template <typename T>
		row_element_ref<T const>::row_element_ref(matrix4f const* owner, uint32_t row)
		: owner_(owner), row_(row)
		{}

		template <typename T>
		auto row_element_ref<T const>::operator[](uint32_t i) const -> cell_element_ref<T const>
		{
			return cell_element_ref<T const>(owner_, row_, i);
		}
	}





	//=====================================================================
	// matrix4f
	//=====================================================================
	matrix4f::matrix4f()
	{
	}

	matrix4f::matrix4f(matrix4f const& rhs)
	{
		sd_[0] = rhs.sd_[0];
		sd_[1] = rhs.sd_[1];
		sd_[2] = rhs.sd_[2];
		sd_[3] = rhs.sd_[3];
	}

#ifdef ATMA_MATH_USE_SSE
	matrix4f::matrix4f(__m128 const& r0, __m128 const& r1, __m128 const& r2, __m128 const& r3)
	{
		sd_[0] = r0;
		sd_[1] = r1;
		sd_[2] = r2;
		sd_[3] = r3;
	}
#endif

	auto matrix4f::operator[](uint32_t i) -> impl::row_element_ref<float>
	{
		return {this, i};
	}

	auto matrix4f::operator[](uint32_t i) const -> impl::row_element_ref<float const>
	{
		return {this, i};
	}

#ifdef ATMA_MATH_USE_SSE
	auto matrix4f::xmmd(uint32_t i) const -> __m128 const&
	{
		return sd_[i];
	}
#endif

	
	auto matrix4f::transpose() -> void
	{
		__m128 t0 = _mm_shuffle_ps(sd_[0], sd_[1], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t1 = _mm_shuffle_ps(sd_[0], sd_[1], _MM_SHUFFLE(3, 2, 3, 2));
		__m128 t2 = _mm_shuffle_ps(sd_[2], sd_[3], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t3 = _mm_shuffle_ps(sd_[2], sd_[3], _MM_SHUFFLE(3, 2, 3, 2));
		
		sd_[0] = t0;
		sd_[1] = t1;
		sd_[2] = t2;
		sd_[3] = t3;
	}

	auto matrix4f::transposed() const -> matrix4f
	{
		__m128 t0 = _mm_shuffle_ps(sd_[0], sd_[1], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t1 = _mm_shuffle_ps(sd_[0], sd_[1], _MM_SHUFFLE(3, 2, 3, 2));
		__m128 t2 = _mm_shuffle_ps(sd_[2], sd_[3], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t3 = _mm_shuffle_ps(sd_[2], sd_[3], _MM_SHUFFLE(3, 2, 3, 2));
		
		return matrix4f(
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(3, 1, 3, 1)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 1, 3, 1))
		);
	}

	inline auto matrix4f::invert() -> void
	{
		auto t = this->transposed();
		
		__m128 v00 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v02 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(0), _MM_SHUFFLE(2, 0, 2, 0));
		__m128 v10 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v11 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v12 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(1), _MM_SHUFFLE(3, 1, 3, 1));
		__m128 d0 = _mm_mul_ps(v00, v10);
		__m128 d1 = _mm_mul_ps(v01, v11);
		__m128 d2 = _mm_mul_ps(v02, v12);

		v00 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(3, 2, 3, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(3, 2, 3, 2));
		v02 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(0), _MM_SHUFFLE(3, 1, 3, 1));
		v10 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 1, 0, 0));
		v11 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(1, 1, 0, 0));
		v12 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(1), _MM_SHUFFLE(2, 0, 2, 0));
		d0 = _mm_sub_ps(d0, _mm_mul_ps(v00, v10));
		d1 = _mm_sub_ps(d1, _mm_mul_ps(v01, v11));
		d2 = _mm_sub_ps(d2, _mm_mul_ps(v02, v12));

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 1, 3, 1));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(0), _MM_SHUFFLE(1, 0, 2, 1));
		v10 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(0, 3, 0, 2));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(0, 1, 0, 2));
		v11 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(2, 1, 2, 1));
		__m128 v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 3, 3, 1));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(1, 0, 2, 1));
		v12 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(0, 3, 0, 2));
		__m128 v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(0, 1, 0, 2));
		v13 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(2, 1, 2, 1));

		__m128 c0 = _mm_mul_ps(v00, v10);
		__m128 c2 = _mm_mul_ps(v01, v11);
		__m128 c4 = _mm_mul_ps(v02, v12);
		__m128 c6 = _mm_mul_ps(v03, v13);

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(0, 0, 1, 0));
		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(2, 1, 3, 2));
		v10 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(1, 3, 2, 3));
		v11 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(0, 2, 1, 2));

		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 2, 1, 0));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(2, 1, 3, 2));
		v12 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(2, 1, 0, 3));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(1, 3, 2, 3));
		v13 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(0, 2, 1, 2));

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		c0 = _mm_sub_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_sub_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

		v00 = _mm_shuffle_ps(t.xmmd(1), t.xmmd(1), _MM_SHUFFLE(0, 3, 0, 3));
		v10 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 2, 2));
		v10 = _mm_shuffle_ps(v10, v10, _MM_SHUFFLE(0, 2, 3, 0));
		v01 = _mm_shuffle_ps(t.xmmd(0), t.xmmd(0), _MM_SHUFFLE(2, 0, 3, 1));
		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 3, 0));
		v11 = _mm_shuffle_ps(v11, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v02 = _mm_shuffle_ps(t.xmmd(3), t.xmmd(3), _MM_SHUFFLE(0, 3, 0, 3));
		v12 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 2, 2));
		v12 = _mm_shuffle_ps(v12, v12, _MM_SHUFFLE(0, 2, 3, 0));
		v03 = _mm_shuffle_ps(t.xmmd(2), t.xmmd(2), _MM_SHUFFLE(2, 0, 3, 1));
		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 3, 0));
		v13 = _mm_shuffle_ps(v13, v13, _MM_SHUFFLE(2, 1, 0, 3));

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		__m128 c1 = _mm_sub_ps(c0, v00);
		__m128 c3 = _mm_add_ps(c2, v01);
		__m128 c5 = _mm_sub_ps(c4, v02);
		__m128 c7 = _mm_add_ps(c6, v03);
		c0 = _mm_add_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_add_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

		c0 = _mm_shuffle_ps(c0, c1, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c3, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c5, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c7, _MM_SHUFFLE(3, 1, 2, 0));
		c0 = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(3, 1, 2, 0));
		c2 = _mm_shuffle_ps(c2, c2, _MM_SHUFFLE(3, 1, 2, 0));
		c4 = _mm_shuffle_ps(c4, c4, _MM_SHUFFLE(3, 1, 2, 0));
		c6 = _mm_shuffle_ps(c6, c6, _MM_SHUFFLE(3, 1, 2, 0));

		// determinate
		__m128 det = _mm_dp_ps(c0, t.xmmd(0), 0xff);
		// one over determinate
		det = _mm_div_ps(xmmd_one_ps, det);

		sd_[0] = _mm_mul_ps(c0, det);
		sd_[1] = _mm_mul_ps(c2, det);
		sd_[2] = _mm_mul_ps(c4, det);
		sd_[3] = _mm_mul_ps(c6, det);
	}


//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================