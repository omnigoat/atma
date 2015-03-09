#pragma once

#include <atma/math/vector4f.hpp>
#include <atma/math/aabb.hpp>

namespace atma { namespace math {

	struct triangle_t
	{
		triangle_t()
		{}

		triangle_t(vector4f const& v0, vector4f const& v1, vector4f const& v2)
			: v0(v0), v1(v1), v2(v2)
		{}

		vector4f v0, v1, v2;

		auto edge0() const -> vector4f { return v1 - v0; }
		auto edge1() const -> vector4f { return v2 - v1; }
		auto edge2() const -> vector4f { return v0 - v2; }

		auto normal() const -> vector4f { return cross_product(edge0(), edge1()); }

		auto aabb() const -> aabb_t
		{
			return aabb_t::from_minmax(
				point4f(
					std::min({v0.x, v1.x, v2.x}),
					std::min({v0.y, v1.y, v2.y}),
					std::min({v0.z, v1.z, v2.z})),
				point4f(
					std::max({v0.x, v1.x, v2.x}),
					std::max({v0.y, v1.y, v2.y}),
					std::max({v0.z, v1.z, v2.z})));
		}
	};

}}
