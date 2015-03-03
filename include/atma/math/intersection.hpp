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

	inline auto intersect_aabc_triangle2(aabc_t const& box, triangle_t const& tri) -> bool
	{
		// bounding-box test
		//if (!intersect_aabb_aabb(tri.aabb(), box))
		//	return false;
		auto p  = box.min_point();
		auto pm = box.max_point();

		auto tmin2 = point4f(std::min(tri.v0.x, tri.v1.x), std::min(tri.v0.y, tri.v1.y), std::min(tri.v0.z, tri.v1.z));
		auto tmin  = point4f(std::min(tri.v2.x, tmin2.x), std::min(tri.v2.y, tmin2.y), std::min(tri.v2.z, tmin2.z));
		auto tmax2 = point4f(std::max(tri.v0.x, tri.v1.x), std::max(tri.v0.y, tri.v1.y), std::max(tri.v0.z, tri.v1.z));
		auto tmax  = point4f(std::max(tri.v2.x, tmax2.x), std::max(tri.v2.y, tmax2.y), std::max(tri.v2.z, tmax2.z));

		// x-axis
		if (tmax.x < p.x || tmax.y < p.y || tmax.z < p.z || pm.x < tmin.x || pm.y < tmin.y || pm.z < tmin.z)
			return false;

		// triangle-normal & edge normals
		auto n = cross_product(tri.v1 - tri.v0, tri.v2 - tri.v0);

		// delta-p, the vector of (min-point, max-point) of the bounding-box
		vector4f dp = box.max_point() - p;

		// critical-point
		auto c = point4f(
			n.x > 0.f ? dp.x : 0.f,
			n.y > 0.f ? dp.y : 0.f,
			n.z > 0.f ? dp.z : 0.f);

		// d1 & d2
		auto d1 = dot_product(n, c - tri.v0);
		auto d2 = dot_product(n, dp - c - tri.v0);

		// test for plane-box overlap
		if ((dot_product(n, p) + d1) * (dot_product(n, p) + d2) > 0.f)
			return false;


		auto pxy = vector4f(p.x, p.y, 0.f, 0.f);
		auto pxz = vector4f(p.x, p.z, 0.f, 0.f);
		auto pyz = vector4f(p.y, p.z, 0.f, 0.f);

		// xy-plane projection-overlap
		auto nxy = [](vector4f const& v0, vector4f const& v1, vector4f const& v2)
		{
			auto AB = vector4f {v1 - v0};
			auto AC = vector4f {v2 - v0};
			auto N = vector4f {-AB.y, AB.x, 0.f, 0.f};
			if (dot_product(N, vector4f {AC.x, AC.y, 0.f, 0.f}) < 0.f)
				N = N * -1;
			return N;
		};

		vector4f ne0xy = nxy(tri.v0, tri.v1, tri.v2); // vector4f(-tri.edge0().y, tri.edge0().x, 0.f, 0.f) * (n.z < 0.f ? -1.f : 1.f);
		vector4f ne1xy = nxy(tri.v1, tri.v2, tri.v0); // vector4f(-tri.edge1().y, tri.edge1().x, 0.f, 0.f) * (n.z < 0.f ? -1.f : 1.f);
		vector4f ne2xy = nxy(tri.v2, tri.v0, tri.v1); // vector4f(-tri.edge2().y, tri.edge2().x, 0.f, 0.f) * (n.z < 0.f ? -1.f : 1.f);

		auto  v0xy   = math::vector4f{tri.v0.x, tri.v0.y, 0.f, 0.f};
		auto  v1xy   = math::vector4f{tri.v1.x, tri.v1.y, 0.f, 0.f};
		auto  v2xy   = math::vector4f{tri.v2.x, tri.v2.y, 0.f, 0.f};

		float de0xy = -dot_product(ne0xy, v0xy) + std::max(0.f, dp.x * ne0xy.x) + std::max(0.f, dp.y * ne0xy.y);
		float de1xy = -dot_product(ne1xy, v1xy) + std::max(0.f, dp.x * ne1xy.x) + std::max(0.f, dp.y * ne1xy.y);
		float de2xy = -dot_product(ne2xy, v2xy) + std::max(0.f, dp.x * ne2xy.x) + std::max(0.f, dp.y * ne2xy.y);

		if ((dot_product(ne0xy, pxy) + de0xy) < 0.f || (dot_product(ne1xy, pxy) + de1xy) < 0.f || (dot_product(ne2xy, pxy) + de2xy) < 0.f)
			return false;


		// xz-plane projection overlap
		auto nxz = [](vector4f const& v0, vector4f const& v1, vector4f const& v2)
		{
			auto AB = vector4f {v1 - v0};
			auto AC = vector4f {v2 - v0};
			auto N = vector4f {-AB.z, AB.x, 0.f, 0.f};
			if (dot_product(N, vector4f {AC.x, AC.z, 0.f, 0.f}) < 0.f)
				N = N * -1;
			return N;
		};

		vector4f ne0xz = nxz(tri.v0, tri.v1, tri.v2);
		vector4f ne1xz = nxz(tri.v1, tri.v2, tri.v0);
		vector4f ne2xz = nxz(tri.v2, tri.v0, tri.v1);

		//vector4f ne0xz = vector4f(-tri.edge0().z, tri.edge0().x, 0.f, 0.f) * (n.y < 0.f ? -1.f : 1.f);
		//vector4f ne1xz = vector4f(-tri.edge1().z, tri.edge1().x, 0.f, 0.f) * (n.y < 0.f ? -1.f : 1.f);
		//vector4f ne2xz = vector4f(-tri.edge2().z, tri.edge2().x, 0.f, 0.f) * (n.y < 0.f ? -1.f : 1.f);

		auto  v0xz   = math::vector4f{tri.v0.x, tri.v0.z, 0.f, 0.f};
		auto  v1xz   = math::vector4f{tri.v1.x, tri.v1.z, 0.f, 0.f};
		auto  v2xz   = math::vector4f{tri.v2.x, tri.v2.z, 0.f, 0.f};

		float de0xz = -dot_product(ne0xz, v0xz) + std::max(0.f, dp.x * ne0xz.x) + std::max(0.f, dp.z * ne0xz.y);
		float de1xz = -dot_product(ne1xz, v1xz) + std::max(0.f, dp.x * ne1xz.x) + std::max(0.f, dp.z * ne1xz.y);
		float de2xz = -dot_product(ne2xz, v2xz) + std::max(0.f, dp.x * ne2xz.x) + std::max(0.f, dp.z * ne2xz.y);

		if ((dot_product(ne0xz, pxz) + de0xz) < 0.f || (dot_product(ne1xz, pxz) + de1xz) < 0.f || (dot_product(ne2xz, pxz) + de2xz) < 0.f)
			return false;


		// yz-plane projection overlap
		auto nyz = [](vector4f const& v0, vector4f const& v1, vector4f const& v2)
		{
			auto AB = vector4f{v1 - v0};
			auto AC = vector4f{v2 - v0};
			auto N = vector4f{-AB.z, AB.y, 0.f, 0.f};
			if (dot_product(N, vector4f{AC.y, AC.z, 0.f, 0.f}) < 0.f)
				N = N * -1;
			return N;
		};

		vector4f ne0yz = nyz(tri.v0, tri.v1, tri.v2); // vector4f(-tri.edge0().z, tri.edge0().y, 0.f, 0.f) * (n.x < 0.f ? -1.f : 1.f);
		vector4f ne1yz = nyz(tri.v1, tri.v2, tri.v0); // vector4f(-tri.edge1().z, tri.edge1().y, 0.f, 0.f) * (n.x < 0.f ? -1.f : 1.f);
		vector4f ne2yz = nyz(tri.v2, tri.v0, tri.v1); // vector4f(-tri.edge2().z, tri.edge2().y, 0.f, 0.f) * (n.x < 0.f ? -1.f : 1.f);

		auto  v0yz   = math::vector4f{tri.v0.y, tri.v0.z, 0.f, 0.f};
		auto  v1yz   = math::vector4f{tri.v1.y, tri.v1.z, 0.f, 0.f};
		auto  v2yz   = math::vector4f{tri.v2.y, tri.v2.z, 0.f, 0.f};

		float de0yz = -dot_product(ne0yz, v0yz) + std::max(0.f, dp.y * ne0yz.x) + std::max(0.f, dp.z * ne0yz.y);
		float de1yz = -dot_product(ne1yz, v1yz) + std::max(0.f, dp.y * ne1yz.x) + std::max(0.f, dp.z * ne1yz.y);
		float de2yz = -dot_product(ne2yz, v2yz) + std::max(0.f, dp.y * ne2yz.x) + std::max(0.f, dp.z * ne2yz.y);

		if ((dot_product(ne0yz, pyz) + de0yz) < 0.f || (dot_product(ne1yz, pyz) + de1yz) < 0.f || (dot_product(ne2yz, pyz) + de2yz) < 0.f)
			return false;


		return true;
	}


	inline auto intersect_aabc_triangle(aabc_t const& aabb, triangle_t const& tri) -> bool
	{
		return intersect_aabb_triangle(aabb_t {aabb.origin(), 0.5f * vector4f{aabb.radius(), aabb.radius(), aabb.radius(), 0.f}}, tri);
	}

} }

