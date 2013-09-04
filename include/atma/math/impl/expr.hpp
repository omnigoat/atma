//=====================================================================
//
//=====================================================================
#ifndef ATMA_MATH_IMPL_EXPR_HPP
#define ATMA_MATH_IMPL_EXPR_HPP
//=====================================================================
#include <atma/math/impl/element_type_of.hpp>
#include <xmmintrin.h>
//=====================================================================
namespace atma {
namespace math {
namespace impl {
//=====================================================================
	
	template <typename R, typename OPER>
	struct expr
	{
#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 {
			return static_cast<OPER const*>(this)->xmmd();
		}
#else
		auto element(uint32_t i) const
		-> typename element_type_of<OPER>::type
		{
			return static_cast<OPER const*>(this)->element(i);
		}
#endif
	};

//=====================================================================
} // namespace impl
} // namespace math
} // namespace atma
//=====================================================================
#endif
//=====================================================================
