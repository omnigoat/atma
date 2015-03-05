#pragma once

#include <atma/math/impl/element_type_of.hpp>


namespace atma { namespace math { namespace impl {

	template <typename R, typename OPER>
	struct expr
	{
#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 {
			return static_cast<OPER const*>(this)->xmmd();
		}
#else
		auto element(uint32 i) const
		-> typename element_type_of<OPER>::type
		{
			return static_cast<OPER const*>(this)->element(i);
		}
#endif
	};



	// vector4f application
	template <typename OPER>
	struct expr<vector4f, OPER>
	{
#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128 {
			return static_cast<OPER const*>(this)->xmmd();
		}
#endif
		auto element(uint32 i) const
			-> typename element_type_of<OPER>::type
		{
			return static_cast<OPER const*>(this)->element(i);
		}
	};


	
} } }
