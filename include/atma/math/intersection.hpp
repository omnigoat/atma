#pragma once

#include <atma/math/aabb.hpp>
#include <atma/math/aabc.hpp>
#include <atma/math/triangle.hpp>


namespace atma { namespace math {

	inline auto intersect_aabb_triangle(aabb_t const& aabb, triangle_t const& tri) -> bool
	{
		auto project = [](vector4f const& axis, vector4f const* points, size_t point_count, float& min, float& max) {
			min = FLT_MAX;
			max = -FLT_MAX;

			for (auto p = points; p != points + point_count; ++p) {
				float v = math::dot_product(axis, *p);
				if (v < min) min = v;
				if (v > max) max = v;
			}
		};


		float box_min = 0.f, box_max = 0.f;
		float tri_min = 0.f, tri_max = 0.f;

		math::vector4f const tri_edges[3] ={
			tri.edge0(),
			tri.edge1(),
			tri.edge2()
		};

		auto box_verts = aabb.compute_corners();

		math::vector4f const box_normals[3] ={
			math::vector4f{1.f, 0.f, 0.f, 1.f},
			math::vector4f{0.f, 1.f, 0.f, 1.f},
			math::vector4f{0.f, 0.f, 1.f, 1.f},
		};


		// test box normals
		{
			for (int i = 0; i != 3; ++i)
			{
				project(box_normals[i], &tri.v0, 3, tri_min, tri_max);
				if (tri_max < aabb.origin().components[i] - aabb.extents().components[i] || tri_min > aabb.origin().components[i] + aabb.extents().components[i])
					return false;
			}
		}

		// test the triangle normal
		{
			auto tri_normal = math::cross_product(tri.v1 - tri.v0, tri.v2 - tri.v0);
			auto tri_offset = math::dot_product(tri_normal, tri.v0);

			project(tri_normal, &box_verts[0], 8, box_min, box_max);

			if (box_max < tri_offset || box_min > tri_offset)
				return false;
		}


		// test nine-edges
		for (int i = 0; i != 3; ++i)
		{
			for (int j = 0; j != 3; ++j)
			{
				auto axis = math::cross_product(tri_edges[i], box_normals[j]);
				project(axis, &box_verts[0], 8, box_min, box_max);
				project(axis, &tri.v0, 3, tri_min, tri_max);
				if (box_max <= tri_min || box_min >= tri_max)
					return false;
			}
		}


		return true;
	}




	inline auto intersect_aabc_triangle(aabc_t const& aabb, triangle_t const& tri) -> bool
	{
		return intersect_aabb_triangle(aabb_t {aabb.origin(), 0.5f * vector4f{aabb.radius(), aabb.radius(), aabb.radius(), 0.f}}, tri);
	}

} }

