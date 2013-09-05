//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_MATRIX_DECLARATION_HPP
#define ATMA_MATH_IMPL_MATRIX_DECLARATION_HPP
//=====================================================================
#include <array>
#include <numeric>
#include <type_traits>
#include <initializer_list>
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	struct matrix4f
	{
		static auto identity() -> matrix4f;

		// constructors
		matrix4f();
		matrix4f(matrix4f const&);
		// matrix4f(vector4f const& r0, ...)
#ifdef ATMA_MATH_USE_SSE
		explicit matrix4f(__m128 const& r0, __m128 const& r1, __m128 const& r2, __m128 const& r3);
#endif

		template <typename T>
		struct cell_element_ref
		{
			cell_element_ref(matrix4f* owner, uint32_t row, uint32_t col)
			: owner_(owner), row_(row), col_(col)
			{}

			auto operator = (float rhs) -> float
			{
				return owner_->rmd_[row_].m128_f32[col_] = rhs;
			}

			operator float() {
				return owner_->xmmd(row_).m128_f32[col_];
			}

		private:
			matrix4f* owner_;
			uint32_t row_, col_;
		};

		template <typename T>
		struct cell_element_ref<T const>
		{
			cell_element_ref(matrix4f const* owner, uint32_t row, uint32_t col)
			: owner_(owner), row_(row), col_(col)
			{}

			operator float() {
				return owner_->xmmd(row_).m128_f32[col_];
			}

		private:
			matrix4f const* owner_;
			uint32_t row_, col_;
		};


		template <typename T>
		struct row_element_ref
		{
			row_element_ref(matrix4f* owner, uint32_t row)
			: owner_(owner), row_(row)
			{}

			auto operator[](int i) -> cell_element_ref<T>
			{
				return cell_element_ref<T>(owner_, row_, i);
			}

			auto operator[](uint32_t i) const -> cell_element_ref<T const>
			{
				return cell_element_ref<T const>(owner_, row_, i);
			}

		private:
			matrix4f* owner_;
			uint32_t row_;
		};

		template <typename T>
		struct row_element_ref<T const>
		{
			row_element_ref(matrix4f const* owner, uint32_t row)
			: owner_(owner), row_(row)
			{}

			auto operator[](uint32_t i) const -> cell_element_ref<T const>
			{
				return cell_element_ref<T const>(owner_, row_, i);
			}

		private:
			matrix4f const* owner_;
			uint32_t row_;
		};

		// operators
		auto operator = (matrix4f const&) -> matrix4f&;
		auto operator[](uint32_t i) -> row_element_ref<float> { return {this, i}; }
		auto operator[](uint32_t i) const -> row_element_ref<float const> { return {this, i}; }

		// computation
		auto transposed() const -> matrix4f;
		auto inverted() const -> matrix4f;
		
		// mutators
		auto set(uint32_t r, uint32_t c, float v) -> void;
		auto transpose() -> void;
		auto invert() -> void;
	

#ifdef ATMA_MATH_USE_SSE
		auto xmmd(uint32_t i) const -> __m128 const&;
#endif

	private:
#ifdef ATMA_MATH_USE_SSE
		__m128 rmd_[4];
#else
		float fpd_[4][4];
#endif
	};


	//=====================================================================
	// operators
	//=====================================================================
	auto operator * (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f;
	auto operator + (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f;
	auto operator - (matrix4f const& lhs, matrix4f const& rhs) -> matrix4f;
	
	auto operator * (matrix4f const& lhs, vector4f const& rhs) -> vector4f;
	auto operator * (matrix4f const& lhs, float f) -> matrix4f;
	

	//=====================================================================
	// functions
	//=====================================================================
	// manipulation
	auto transpose(matrix4f const& x) -> matrix4f;
	auto inverse(matrix4f const& x) -> matrix4f;

	// view
	auto look_along(vector4f const& position, vector4f const& direction, vector4f const& up) -> matrix4f;
	auto look_to(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f;

	// projection (left-handed)
	auto pespective(float width, float height, float near, float far) -> matrix4f;



//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
