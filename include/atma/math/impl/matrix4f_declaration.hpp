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
	//=====================================================================
	__declspec(align(64))
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



		// operators
		auto operator = (matrix4f const&)-> matrix4f&;
		auto operator[](uint32_t i) -> impl::row_element_ref<float>;
		auto operator[](uint32_t i) const -> impl::row_element_ref<float const>;

		// computation
		auto transposed() const -> matrix4f;
		auto inverted() const -> matrix4f;

		// mutators
		auto transpose() -> void;
		auto invert() -> void;


#ifdef ATMA_MATH_USE_SSE
		auto xmmd(uint32_t i) const -> __m128 const&;
#endif

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
			cell_element_ref(matrix4f* owner, uint32_t row, uint32_t col);
			auto operator = (float rhs) -> float;
			operator float();

		private:
			matrix4f* owner_;
			uint32_t row_, col_;
		};

		template <typename T>
		struct cell_element_ref<T const>
		{
			cell_element_ref(matrix4f const* owner, uint32_t row, uint32_t col);
			operator float();

		private:
			matrix4f const* owner_;
			uint32_t row_, col_;
		};


		template <typename T>
		struct row_element_ref
		{
			row_element_ref(matrix4f* owner, uint32_t row);
			auto operator[](uint32_t i) -> cell_element_ref<T>;
			auto operator[](uint32_t i) const -> cell_element_ref<T const>;

		private:
			matrix4f* owner_;
			uint32_t row_;
		};

		template <typename T>
		struct row_element_ref<T const>
		{
			row_element_ref(matrix4f const* owner, uint32_t row);
			auto operator[](uint32_t i) const -> cell_element_ref<T const>;

		private:
			matrix4f const* owner_;
			uint32_t row_;
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
	auto look_to(vector4f const& position, vector4f const& target, vector4f const& up) -> matrix4f;

	// projection (left-handed)
	auto pespective(float width, float height, float near, float far) -> matrix4f;



//=====================================================================
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
