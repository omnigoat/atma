#pragma once

#include <atma/math/vector4f.hpp>


namespace atma { namespace math {

	struct triangle_t
	{
		triangle_t()
		{}

		triangle_t(math::vector4f const& v0, math::vector4f const& v1, math::vector4f const& v2)
			: v0(v0), v1(v1), v2(v2)
		{}

		math::vector4f v0, v1, v2;

		auto edge0() const -> math::vector4f { return v1 - v0; }
		auto edge1() const -> math::vector4f { return v2 - v1; }
		auto edge2() const -> math::vector4f { return v0 - v2; }
	};

}}
