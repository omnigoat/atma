#pragma once

#include <atma/math/constants.hpp>
#include <atma/math/scalar.hpp>
#include <atma/math/vector4f.hpp>
#include <atma/types.hpp>

namespace atma { namespace math {

	struct matrix4f;
	namespace detail
	{
		template <typename T> struct cell_element_ref;
		template <typename T> struct row_element_ref;
	}




	//=====================================================================
	// matrix4f
	//=====================================================================
	__declspec(align(64))
	struct matrix4f
	{
		// constructors
		matrix4f();
		matrix4f(matrix4f const&) = default;
		matrix4f(__m128 r0, __m128 r1, __m128 r2, __m128 r3);
		matrix4f(vector4f const&, vector4f const&, vector4f const&, vector4f const&);

		auto operator = (matrix4f const&) -> matrix4f& = default;
		auto operator [](int i)           -> detail::row_element_ref<float>;
		auto operator [](int i) const     -> detail::row_element_ref<float const>;

		static auto identity()                 -> matrix4f;
		static auto scale(float)               -> matrix4f;
		static auto scale(float, float, float) -> matrix4f;
		static auto translate(vector4f const&) -> matrix4f;

		__m128 xmmdata[4];

		template <typename T> friend struct detail::row_element_ref;
		template <typename T> friend struct detail::cell_element_ref;
	};




	//=====================================================================
	// row_element_ref / cell_element_ref
	//=====================================================================
	namespace detail
	{
		template <typename T>
		struct row_element_ref
		{
			row_element_ref(matrix4f* owner, int row);
			auto operator[](int i) -> cell_element_ref<T>;
			auto operator[](int i) const -> cell_element_ref<T const>;
			auto operator = (vector4f const&) -> row_element_ref&;
			operator vector4f() const;

		private:
			matrix4f* owner_;
			int row_;
		};

		template <typename T>
		struct row_element_ref<T const>
		{
			row_element_ref(matrix4f const* owner, int row);
			auto operator[](int i) const -> cell_element_ref<T const>;
			operator vector4f() const;

		private:
			matrix4f const* owner_;
			int row_;
		}; 


		template <typename T>
		struct cell_element_ref
		{
			cell_element_ref(matrix4f* owner, int row, int col);
			auto operator = (float) -> cell_element_ref&;
			operator float() const;

		private:
			matrix4f* owner_;
			int row_, col_;
		};

		template <typename T>
		struct cell_element_ref<T const>
		{
			cell_element_ref(matrix4f const* owner, int row, int col);
			operator float() const;

		private:
			matrix4f const* owner_;
			int row_, col_;
		};
	}








	//=====================================================================
	// implementation
	//=====================================================================
	inline matrix4f::matrix4f()
	{
		xmmdata[0] = xmmd_zero_ps;
		xmmdata[1] = xmmd_zero_ps;
		xmmdata[2] = xmmd_zero_ps;
		xmmdata[3] = xmmd_zero_ps;
	}

	inline matrix4f::matrix4f(__m128 r0, __m128 r1, __m128 r2, __m128 r3)
	{
		xmmdata[0] = r0;
		xmmdata[1] = r1;
		xmmdata[2] = r2;
		xmmdata[3] = r3;
	}

	inline auto matrix4f::operator[](int i) -> detail::row_element_ref<float>
	{
		return detail::row_element_ref<float>{this, i};
	}

	inline auto matrix4f::operator[](int i) const -> detail::row_element_ref<float const>
	{
		return detail::row_element_ref<float const>{this, i};
	}

	inline auto matrix4f::identity() -> matrix4f
	{
		return matrix4f(xmmd_identity_r0_ps, xmmd_identity_r1_ps, xmmd_identity_r2_ps, xmmd_identity_r3_ps);
	}

	inline auto matrix4f::scale(float s) -> matrix4f
	{
		return scale(s, s, s);
	}

	inline auto matrix4f::scale(float x, float y, float z) -> matrix4f
	{
		return matrix4f{
			_mm_set_ps(0.f, 0.f, 0.f, x),
			_mm_set_ps(0.f, 0.f, y, 0.f),
			_mm_set_ps(0.f, z, 0.f, 0.f),
			_mm_set_ps(1.f, 0.f, 0.f, 0.f)};
	}

	inline auto matrix4f::translate(vector4f const& v) -> matrix4f
	{
		auto v2 = v;
		v2.w = 1.f;
		return matrix4f{xmmd_identity_r0_ps, xmmd_identity_r1_ps, xmmd_identity_r2_ps, v2.xmmdata};
	}


	//=====================================================================
	// row_element_ref / cell_element_ref implementation
	//=====================================================================
	namespace detail
	{
		// row-element-ref
		template <typename T>
		inline row_element_ref<T>::row_element_ref(matrix4f* owner, int row)
			: owner_(owner), row_(row)
		{
		}

		template <typename T>
		inline auto row_element_ref<T>::operator[](int i) -> cell_element_ref<T>
		{
			return cell_element_ref<T>(owner_, row_, i);
		}

		template <typename T>
		inline auto row_element_ref<T>::operator[](int i) const -> cell_element_ref<T const>
		{
			return cell_element_ref<T const>(owner_, row_, i);
		}

		template <typename T>
		inline auto row_element_ref<T>::operator = (vector4f const& rhs) -> row_element_ref&
		{
			owner_->xmmdata[row_] = rhs.xmmdata;
			return *this;
		}

		template <typename T>
		inline row_element_ref<T>::operator vector4f() const
		{
			return vector4f{owner_->xmmdata[row_]};
		}

		// row-element-ref (const)
		template <typename T>
		inline row_element_ref<T const>::row_element_ref(matrix4f const* owner, int row)
			: owner_(owner), row_(row)
		{
		}

		template <typename T>
		inline auto row_element_ref<T const>::operator[](int i) const -> cell_element_ref<T const>
		{
			return cell_element_ref<T const>(owner_, row_, i);
		}

		template <typename T>
		inline row_element_ref<T const>::operator vector4f() const
		{
			return vector4f{owner_->xmmdata[row_]};
		}

		// cell-element-ref
		template <typename T>
		inline cell_element_ref<T>::cell_element_ref(matrix4f* owner, int row, int col)
			: owner_(owner), row_(row), col_(col)
		{
		}

		template <typename T>
		inline auto cell_element_ref<T>::operator = (float rhs) -> cell_element_ref&
		{
			owner_->xmmdata[row_].m128_f32[col_] = rhs;
			return *this;
		}

		template <typename T>
		inline cell_element_ref<T>::operator float() const
		{
			return owner_->xmmdata[row_].m128_f32[col_];
		}

		// cell-element-ref (const)
		template <typename T>
		inline cell_element_ref<T const>::cell_element_ref(matrix4f const* owner, int row, int col)
			: owner_(owner), row_(row), col_(col)
		{
		}

		template <typename T>
		inline cell_element_ref<T const>::operator float() const
		{
			return owner_->xmmdata[row_].m128_f32[col_];
		}
	}




	//=====================================================================
	// operators
	//=====================================================================
	inline auto operator * (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
		auto r00 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[0], lhs.xmmdata[0], _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmdata[0]);
		auto r01 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[0], lhs.xmmdata[0], _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmdata[1]);
		auto r02 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[0], lhs.xmmdata[0], _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmdata[2]);
		auto r03 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[0], lhs.xmmdata[0], _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmdata[3]);
		auto r0 = _mm_add_ps(_mm_add_ps(r00, r01), _mm_add_ps(r02, r03));

		auto r10 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[1], lhs.xmmdata[1], _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmdata[0]);
		auto r11 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[1], lhs.xmmdata[1], _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmdata[1]);
		auto r12 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[1], lhs.xmmdata[1], _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmdata[2]);
		auto r13 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[1], lhs.xmmdata[1], _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmdata[3]);
		auto r1 = _mm_add_ps(_mm_add_ps(r10, r11), _mm_add_ps(r12, r13));

		auto r20 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[2], lhs.xmmdata[2], _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmdata[0]);
		auto r21 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[2], lhs.xmmdata[2], _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmdata[1]);
		auto r22 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[2], lhs.xmmdata[2], _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmdata[2]);
		auto r23 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[2], lhs.xmmdata[2], _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmdata[3]);
		auto r2 = _mm_add_ps(_mm_add_ps(r20, r21), _mm_add_ps(r22, r23));
		
		auto r30 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[3], lhs.xmmdata[3], _MM_SHUFFLE(0, 0, 0, 0)), rhs.xmmdata[0]);
		auto r31 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[3], lhs.xmmdata[3], _MM_SHUFFLE(1, 1, 1, 1)), rhs.xmmdata[1]);
		auto r32 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[3], lhs.xmmdata[3], _MM_SHUFFLE(2, 2, 2, 2)), rhs.xmmdata[2]);
		auto r33 = _mm_mul_ps(_mm_shuffle_ps(lhs.xmmdata[3], lhs.xmmdata[3], _MM_SHUFFLE(3, 3, 3, 3)), rhs.xmmdata[3]);
		auto r3 = _mm_add_ps(_mm_add_ps(r30, r31), _mm_add_ps(r32, r33));

		return matrix4f{r0, r1, r2, r3};
	}

	inline auto operator + (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
		return matrix4f{
			_mm_add_ps(lhs.xmmdata[0], rhs.xmmdata[0]),
			_mm_add_ps(lhs.xmmdata[1], rhs.xmmdata[1]),
			_mm_add_ps(lhs.xmmdata[2], rhs.xmmdata[2]),
			_mm_add_ps(lhs.xmmdata[3], rhs.xmmdata[3])};
	}

	inline auto operator - (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f
	{
		return matrix4f{
			_mm_sub_ps(lhs.xmmdata[0], rhs.xmmdata[0]),
			_mm_sub_ps(lhs.xmmdata[1], rhs.xmmdata[1]),
			_mm_sub_ps(lhs.xmmdata[2], rhs.xmmdata[2]),
			_mm_sub_ps(lhs.xmmdata[3], rhs.xmmdata[3])};
	}

	inline auto operator * (vector4f const& lhs, matrix4f const& rhs) -> vector4f
	{
		auto lhsx = _mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(0, 0, 0, 0));
		auto lhsy = _mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(1, 1, 1, 1));
		auto lhsz = _mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(2, 2, 2, 2));
		auto lhsw = _mm_shuffle_ps(lhs.xmmdata, lhs.xmmdata, _MM_SHUFFLE(3, 3, 3, 3));

		auto lhsmx = _mm_mul_ps(lhsx, rhs.xmmdata[0]);
		auto lhsmy = _mm_mul_ps(lhsy, rhs.xmmdata[1]);
		auto lhsmz = _mm_mul_ps(lhsz, rhs.xmmdata[2]);
		auto lhsmw = _mm_mul_ps(lhsw, rhs.xmmdata[3]);

		return vector4f{_mm_add_ps(_mm_add_ps(lhsmx, lhsmy), _mm_add_ps(lhsmz, lhsmw))};
	}

	inline auto operator * (matrix4f const& lhs, float f) -> matrix4f
	{
		__m128 ss = _am_load_ps(f, f, f, f);

		__m128 r0 = _mm_mul_ps(lhs.xmmdata[0], ss);
		__m128 r1 = _mm_mul_ps(lhs.xmmdata[1], ss);
		__m128 r2 = _mm_mul_ps(lhs.xmmdata[2], ss);
		__m128 r3 = _mm_mul_ps(lhs.xmmdata[3], ss);
		
		return matrix4f(r0, r1, r2, r3);
	}

	inline auto operator / (matrix4f const& lhs, float f) -> matrix4f
	{
		__m128 ss = _am_load_ps(f, f, f, f);

		__m128 r0 = _mm_div_ps(lhs.xmmdata[0], ss);
		__m128 r1 = _mm_div_ps(lhs.xmmdata[1], ss);
		__m128 r2 = _mm_div_ps(lhs.xmmdata[2], ss);
		__m128 r3 = _mm_div_ps(lhs.xmmdata[3], ss);

		return matrix4f(r0, r1, r2, r3);
	}




	//=====================================================================
	// functions
	//=====================================================================
	inline auto transpose(matrix4f const& x) -> matrix4f
	{
		__m128 t0 = _mm_shuffle_ps(x.xmmdata[0], x.xmmdata[1], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t1 = _mm_shuffle_ps(x.xmmdata[0], x.xmmdata[1], _MM_SHUFFLE(3, 2, 3, 2));
		__m128 t2 = _mm_shuffle_ps(x.xmmdata[2], x.xmmdata[3], _MM_SHUFFLE(1, 0, 1, 0));
		__m128 t3 = _mm_shuffle_ps(x.xmmdata[2], x.xmmdata[3], _MM_SHUFFLE(3, 2, 3, 2));
		
		return matrix4f{
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t0, t2, _MM_SHUFFLE(3, 1, 3, 1)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(2, 0, 2, 0)),
			_mm_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 1, 3, 1))};
	}

	inline auto invert(matrix4f const& x) -> matrix4f
	{
		auto t = transpose(x);
		
		__m128 v00 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[2], _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v01 = _mm_shuffle_ps(t.xmmdata[0], t.xmmdata[0], _MM_SHUFFLE(1, 1, 0, 0));
		__m128 v02 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[0], _MM_SHUFFLE(2, 0, 2, 0));
		__m128 v10 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[3], _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v11 = _mm_shuffle_ps(t.xmmdata[1], t.xmmdata[1], _MM_SHUFFLE(3, 2, 3, 2));
		__m128 v12 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[1], _MM_SHUFFLE(3, 1, 3, 1));
		__m128 d0 = _mm_mul_ps(v00, v10);
		__m128 d1 = _mm_mul_ps(v01, v11);
		__m128 d2 = _mm_mul_ps(v02, v12);

		v00 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[2], _MM_SHUFFLE(3, 2, 3, 2));
		v01 = _mm_shuffle_ps(t.xmmdata[0], t.xmmdata[0], _MM_SHUFFLE(3, 2, 3, 2));
		v02 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[0], _MM_SHUFFLE(3, 1, 3, 1));
		v10 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[3], _MM_SHUFFLE(1, 1, 0, 0));
		v11 = _mm_shuffle_ps(t.xmmdata[1], t.xmmdata[1], _MM_SHUFFLE(1, 1, 0, 0));
		v12 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[1], _MM_SHUFFLE(2, 0, 2, 0));
		d0 = _mm_sub_ps(d0, _mm_mul_ps(v00, v10));
		d1 = _mm_sub_ps(d1, _mm_mul_ps(v01, v11));
		d2 = _mm_sub_ps(d2, _mm_mul_ps(v02, v12));

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 1, 3, 1));
		v00 = _mm_shuffle_ps(t.xmmdata[1], t.xmmdata[0], _MM_SHUFFLE(1, 0, 2, 1));
		v10 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(0, 3, 0, 2));
		v01 = _mm_shuffle_ps(t.xmmdata[0], t.xmmdata[0], _MM_SHUFFLE(0, 1, 0, 2));
		v11 = _mm_shuffle_ps(v11, d0, _MM_SHUFFLE(2, 1, 2, 1));
		__m128 v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 3, 3, 1));
		v02 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[3], _MM_SHUFFLE(1, 0, 2, 1));
		v12 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(0, 3, 0, 2));
		__m128 v03 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[2], _MM_SHUFFLE(0, 1, 0, 2));
		v13 = _mm_shuffle_ps(v13, d1, _MM_SHUFFLE(2, 1, 2, 1));

		__m128 c0 = _mm_mul_ps(v00, v10);
		__m128 c2 = _mm_mul_ps(v01, v11);
		__m128 c4 = _mm_mul_ps(v02, v12);
		__m128 c6 = _mm_mul_ps(v03, v13);

		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(0, 0, 1, 0));
		v00 = _mm_shuffle_ps(t.xmmdata[1], t.xmmdata[1], _MM_SHUFFLE(2, 1, 3, 2));
		v10 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v01 = _mm_shuffle_ps(t.xmmdata[0], t.xmmdata[0], _MM_SHUFFLE(1, 3, 2, 3));
		v11 = _mm_shuffle_ps(d0, v11, _MM_SHUFFLE(0, 2, 1, 2));

		v13 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(2, 2, 1, 0));
		v02 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[3], _MM_SHUFFLE(2, 1, 3, 2));
		v12 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(2, 1, 0, 3));
		v03 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[2], _MM_SHUFFLE(1, 3, 2, 3));
		v13 = _mm_shuffle_ps(d1, v13, _MM_SHUFFLE(0, 2, 1, 2));

		v00 = _mm_mul_ps(v00, v10);
		v01 = _mm_mul_ps(v01, v11);
		v02 = _mm_mul_ps(v02, v12);
		v03 = _mm_mul_ps(v03, v13);
		c0 = _mm_sub_ps(c0, v00);
		c2 = _mm_sub_ps(c2, v01);
		c4 = _mm_sub_ps(c4, v02);
		c6 = _mm_sub_ps(c6, v03);

		v00 = _mm_shuffle_ps(t.xmmdata[1], t.xmmdata[1], _MM_SHUFFLE(0, 3, 0, 3));
		v10 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 2, 2));
		v10 = _mm_shuffle_ps(v10, v10, _MM_SHUFFLE(0, 2, 3, 0));
		v01 = _mm_shuffle_ps(t.xmmdata[0], t.xmmdata[0], _MM_SHUFFLE(2, 0, 3, 1));
		v11 = _mm_shuffle_ps(d0, d2, _MM_SHUFFLE(1, 0, 3, 0));
		v11 = _mm_shuffle_ps(v11, v11, _MM_SHUFFLE(2, 1, 0, 3));
		v02 = _mm_shuffle_ps(t.xmmdata[3], t.xmmdata[3], _MM_SHUFFLE(0, 3, 0, 3));
		v12 = _mm_shuffle_ps(d1, d2, _MM_SHUFFLE(3, 2, 2, 2));
		v12 = _mm_shuffle_ps(v12, v12, _MM_SHUFFLE(0, 2, 3, 0));
		v03 = _mm_shuffle_ps(t.xmmdata[2], t.xmmdata[2], _MM_SHUFFLE(2, 0, 3, 1));
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

		__m128 det = _mm_dp_ps(c0, t.xmmdata[0], 0xff);
		det = _mm_div_ps(xmmd_one_ps, det);

		return matrix4f{
			_mm_mul_ps(c0, det),
			_mm_mul_ps(c2, det),
			_mm_mul_ps(c4, det),
			_mm_mul_ps(c6, det)};
	}
	
	inline auto look_along(vector4f const& position, vector4f const& direction, vector4f const& up) -> matrix4f
	{
		vector4f r2 = normalize(direction);
		vector4f r0 = normalize(cross_product(up, r2));
		vector4f r1 = cross_product(r2, r0);
		vector4f npos = vector4f(_mm_mul_ps(position.xmmdata, xmmd_negone_ps));

		__m128 d0 = _mm_dp_ps(r0.xmmdata, npos.xmmdata, 0x7f);
		__m128 d1 = _mm_dp_ps(r1.xmmdata, npos.xmmdata, 0x7f);
		__m128 d2 = _mm_dp_ps(r2.xmmdata, npos.xmmdata, 0x7f);

		auto result = matrix4f{
			_am_select_ps(d0, r0.xmmdata, xmmd_mask_1110_ps),
			_am_select_ps(d1, r1.xmmdata, xmmd_mask_1110_ps),
			_am_select_ps(d2, r2.xmmdata, xmmd_mask_1110_ps),
			xmmd_identity_r3_ps};

		return transpose(result);
	}

	inline auto look_at(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f
	{
		return look_along(position, target - position, up);
	}

	inline auto pespective(float width, float height, float near_plane, float far_plane) -> matrix4f
	{
		float nn = near_plane + near_plane;
		float range = far_plane / (far_plane - near_plane);

		__m128 rmem = _am_load_ps(-range * near_plane, range, nn / height, nn / width);
		__m128 t0 = _mm_setzero_ps();

		__m128 r0 = _mm_move_ss(t0, rmem);
		__m128 r1 = _mm_and_ps(rmem, xmmd_mask_0100_ps);
		rmem = _mm_shuffle_ps(rmem, xmmd_identity_r3_ps, _MM_SHUFFLE(3, 2, 3, 2));
		__m128 r2 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(3, 0, 0, 0));
		__m128 r3 = _mm_shuffle_ps(t0, rmem, _MM_SHUFFLE(2, 1, 0, 0));

		return matrix4f{r0, r1, r2, r3};
	}

	inline auto perspective_fov(float fov, float aspect, float near_plane, float far_plane) -> matrix4f
	{
		float sin_fov, cos_fov;
		retrieve_sin_cos(sin_fov, cos_fov, 0.5f * fov);

		float range = far_plane / (far_plane-near_plane);
		float scale = cos_fov / sin_fov;

		auto values = _am_load_ps(scale, scale / aspect, range, -range * near_plane);
		// range,-range * near_plane,0,1.0f
		auto values2 = _mm_shuffle_ps(values, xmmd_identity_r3_ps, _MM_SHUFFLE(3, 2, 3, 2));

		return matrix4f(
			// scale, 0, 0, 0
			_mm_move_ss(xmmd_zero_ps, values),
			// 0, scale/aspect, 0, 0
			_mm_and_ps(values, xmmd_mask_0100_ps),
			// 0, 0, range, 1.f
			_mm_shuffle_ps(xmmd_zero_ps, values2, _MM_SHUFFLE(3, 0, 0, 0)),
			// 0, 0, -range*near_plane, 0
			_mm_shuffle_ps(xmmd_zero_ps, values2, _MM_SHUFFLE(2, 1, 0, 0))
			);
	}

	inline auto orthographic(float left, float right, float top, float bottom, float near, float far) -> matrix4f
	{
		// todo: SIMD this
		return matrix4f
		{
			vector4f{2.f / (right - 1.f), 0.f, 0.f, 0.f},
			vector4f{0.f, 2.f / (top - bottom), 0.f, 0.f},
			vector4f{0.f, 0.f, 1.f / (far - near), 0.f},
			vector4f{(1.f + right) / (1.f - right), (top+bottom) / (bottom-top), near/(near - far), 1.f}
		};
	}

	inline auto rotation_y(float angle) -> matrix4f
	{
		float sin_angle;
		float cos_angle;
		retrieve_sin_cos(sin_angle, cos_angle, angle);

		auto sin = _mm_set_ss(sin_angle);
		auto cos = _mm_set_ss(cos_angle);

		return matrix4f{
			// cos,0,-sin,0
			_mm_shuffle_ps(cos, _mm_mul_ps(sin, xmmd_neg_x_ps), _MM_SHUFFLE(3, 0, 3, 0)),
			// 0,1,0,0
			xmmd_identity_r1_ps,
			// sin,0,cos,0
			_mm_shuffle_ps(sin, cos, _MM_SHUFFLE(3, 0, 3, 0)),
			// 0,0,0,1
			xmmd_identity_r3_ps};
	}

	inline auto rotation_x(float angle) -> matrix4f
	{
		float sin_angle;
		float cos_angle;
		retrieve_sin_cos(sin_angle, cos_angle, angle);

		auto sin = _mm_set_ss(sin_angle);
		auto cos = _mm_set_ss(cos_angle);

		return matrix4f{
			// 1,0,0,0
			xmmd_identity_r0_ps,
			// 0,cos,sin,0
			_mm_shuffle_ps(cos, sin, _MM_SHUFFLE(3, 0, 0, 3)),
			// 0,-sin,cos,0
			_mm_shuffle_ps(_mm_mul_ps(sin, xmmd_neg_x_ps), cos, _MM_SHUFFLE(3, 0, 0, 3)),
			// 0,0,0,1
			xmmd_identity_r3_ps};
	}


} }

namespace aml = atma::math;
