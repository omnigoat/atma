#pragma once

#ifndef ATMA_MATH_VECTOR4I_SCOPE
#	error "this file needs to be included solely from vector4i.hpp"
#endif


namespace atma { namespace math {

	__declspec(align(16))
	struct vector4i
	{
		vector4i();
		vector4i(int32 x, int32 y, int32 z, int32 w);
		explicit vector4i(__m128i);
		

		auto operator [] (int) const -> int32;
		auto operator *= (int32) -> vector4i&;
		auto operator /= (int32) -> vector4i&;

		template <typename OP> auto operator  = (impl::expr<vector4i, OP> const&) -> vector4i&;
		template <typename OP> auto operator += (impl::expr<vector4i, OP> const&) -> vector4i&;
		template <typename OP> auto operator -= (impl::expr<vector4i, OP> const&) -> vector4i&;

#ifdef ATMA_MATH_USE_SSE
		auto xmmd() const -> __m128i { return xmmdata; }
#endif

		auto is_zero() const -> bool;

#pragma warning(push)
#pragma warning(disable: 4201)
		union {
			struct
			{
				int32 x, y, z, w;
			};

#if defined(ATMA_MATH_USE_SSE)
			__m128i xmmdata;
#endif

			int32 components[4];
		};
#pragma warning(pop)
	};

} }
