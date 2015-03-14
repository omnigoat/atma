#pragma once
//=====================================================================
#ifndef ATMA_MATH_MATRIX4F_SCOPE
#	error "this file needs to be included solely from matrix4.hpp"
#endif
//=====================================================================
#include <atma/math/vector4f.hpp>
#include <atma/assert.hpp>

#include <array>
#include <numeric>
#include <type_traits>
#include <initializer_list>
//=====================================================================
namespace atma {
namespace math {
//=====================================================================
	
	//=====================================================================
	// forward declares
	//=====================================================================
	struct matrix4f;
	namespace impl
	{
		template <typename T> struct cell_element_ref;
		template <typename T> struct row_element_ref;
	}



	//=====================================================================
	// matrix4f
	// ----------
	//   row-major matrices, where all functions/operators 
	//=====================================================================
	__declspec(align(64))
	struct matrix4f
	{
		// constructors
		matrix4f();
		matrix4f(matrix4f const&);
		// matrix4f(vector4f const& r0, ...)
#ifdef ATMA_MATH_USE_SSE
		explicit matrix4f(__m128 const& r0, __m128 const& r1, __m128 const& r2, __m128 const& r3);
#endif

		// operators
		auto operator = (matrix4f const&)-> matrix4f&;
		auto operator[](uint32 i) -> impl::row_element_ref<float>;
		auto operator[](uint32 i) const -> impl::row_element_ref<float const>;

		// computation
		auto transposed() const -> matrix4f;
		auto inverted() const -> matrix4f;

		// mutators
		auto transpose() -> void;
		auto invert() -> void;


#ifdef ATMA_MATH_USE_SSE
		auto xmmd(uint32 i) const -> __m128 const&;
#endif


		static auto identity() -> matrix4f;
		static auto scale(float) -> matrix4f;
		static auto scale(float, float, float) -> matrix4f;
		static auto translate(vector4f const&) -> matrix4f;

	private:
#ifdef ATMA_MATH_USE_SSE
		__m128 sd_[4];
#else
		float fd_[4][4];
#endif

		template <typename T>
		friend struct impl::cell_element_ref;
	};



	//=====================================================================
	// impl
	//=====================================================================
	namespace impl
	{
		template <typename T>
		struct cell_element_ref
		{
			cell_element_ref(matrix4f* owner, uint32 row, uint32 col);
			auto operator = (float rhs) -> float;
			operator float();

		private:
			matrix4f* owner_;
			uint32 row_, col_;
		};

		template <typename T>
		struct cell_element_ref<T const>
		{
			cell_element_ref(matrix4f const* owner, uint32 row, uint32 col);
			operator float();

		private:
			matrix4f const* owner_;
			uint32 row_, col_;
		};


		template <typename T>
		struct row_element_ref
		{
			row_element_ref(matrix4f* owner, uint32 row);
			auto operator[](uint32 i) -> cell_element_ref<T>;
			auto operator[](uint32 i) const -> cell_element_ref<T const>;

		private:
			matrix4f* owner_;
			uint32 row_;
		};

		template <typename T>
		struct row_element_ref<T const>
		{
			row_element_ref(matrix4f const* owner, uint32 row);
			auto operator[](uint32 i) const -> cell_element_ref<T const>;

		private:
			matrix4f const* owner_;
			uint32 row_;
		};
	}


	
	//=====================================================================
	// functions
	//=====================================================================
	// manipulation
	//auto transpose(matrix4f const& x) -> matrix4f;
	//auto inverse(matrix4f const& x) -> matrix4f;

	// view
	auto look_along(vector4f const& position, vector4f const& direction, vector4f const& up) -> matrix4f;
	auto look_at(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f;

	// projection (left-handed)
	auto pespective(float width, float height, float near, float far) -> matrix4f;



//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
