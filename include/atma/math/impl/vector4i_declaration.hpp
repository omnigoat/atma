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
		

		auto operator *= (int32) -> vector4i&;
		auto operator /= (int32) -> vector4i&;

		auto operator  = (vector4i const&) -> vector4i&;
		//template <typename OP> auto operator += (impl::expr<vector4i, OP> const&) -> vector4i&;
		//template <typename OP> auto operator -= (impl::expr<vector4i, OP> const&) -> vector4i&;


		auto is_zero() const -> bool;

#pragma warning(push)
#pragma warning(disable: 4201)
		union {
			struct {
				int32 x, y, z, w;
			};

			__m128i xmmdata;

			int32 components[4];
		};
#pragma warning(pop)
	};

} }
