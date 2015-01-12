#pragma once

#include <type_traits>


namespace atma { namespace math { namespace impl {


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
	inline auto xmmd_of(expr<R, OP> const& expr) -> __m128
	{
		return expr.xmmd();
	}

	inline auto xmmd_of(float x) -> __m128
	{
		return _mm_load_ps1(&x);
	}
#endif

	template <typename T>
	inline auto element_of(T const& x, uint32 i) -> float
	{
		return x[i];
	}

	inline auto element_of(float x, uint32) -> float
	{
		return x;
	}


} } }
