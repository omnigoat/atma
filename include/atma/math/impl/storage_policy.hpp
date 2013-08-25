//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_UTILITY_HPP
#define ATMA_MATH_IMPL_UTILITY_HPP
//=====================================================================
#include <type_traits>
//=====================================================================
namespace atma {
namespace math {
namespace impl {
//=====================================================================
	
	template <typename, typename> struct expr;


	//=====================================================================
	// storage_policy
	// ----------------
	//   stores integrals by value, and all other things by reference. this
	//   means that expressions are only valid as rvalue references.
	//=====================================================================
	template <typename T>
	struct storage_policy {
		typedef T const& type;
	};

	template <>
	struct storage_policy<float> {
		typedef float const type;
	};



	//=====================================================================
	// xmmd_of / element_of
	// ----------------------
	//   returns the __m128 or the element (float) of various things.
	//   automatically promotes a floating-point value to a four-component
	//   __m128 of [f, f, f, f]
	//=====================================================================
#ifdef ATMA_MATH_USE_SSE
	template <typename R, typename OP>
	auto xmmd_of(expr<R, OP> const& expr) -> __m128
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


//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
