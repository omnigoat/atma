#pragma once

#include <atma/math/aabb.hpp>
#include <atma/math/aabc.hpp>
#include <atma/math/triangle.hpp>


namespace atma { namespace math {

	template <typename A, typename B>
	inline auto intersect_aabbs(A const& lhs, B const& rhs) -> bool
	{
		auto ln = lhs.min_point();
		auto lx = lhs.max_point();
		auto rn = rhs.min_point();
		auto rx = rhs.max_point();

		return !(lx.x < rn.x || lx.y < rn.y || lx.z < rn.z || rx.x < ln.x || rx.y < ln.y || rx.z < ln.z);
	}




	template <typename Box>
	inline auto intersect_aabb_triangle(Box const& box, triangle_t const& tri) -> bool
	{
		// bounding-box test
		if (!intersect_aabbs(box, tri.aabb()))
			return false;

		// triangle-normal
		auto n = tri.normal();
		
		// p & delta-p
		vector4f p  = box.min_point();
		vector4f dp = box.max_point() - p;

		// test for triangle-plane/box overlap
		auto c = point4f(
			n.x > 0.f ? dp.x : 0.f,
			n.y > 0.f ? dp.y : 0.f,
			n.z > 0.f ? dp.z : 0.f);

		auto d1 = dot_product(n, c - tri.v0);
		auto d2 = dot_product(n, dp - c - tri.v0);

		if ((dot_product(n, p) + d1) * (dot_product(n, p) + d2) > 0.f)
			return false;


		// xy-plane projection-overlap
		auto xym = (n.z < 0.f ? -1.f : 1.f);
		vector4f ne0xy = vector4f{-tri.edge0().y, tri.edge0().x, 0.f, 0.f} * xym;
		vector4f ne1xy = vector4f{-tri.edge1().y, tri.edge1().x, 0.f, 0.f} * xym;
		vector4f ne2xy = vector4f{-tri.edge2().y, tri.edge2().x, 0.f, 0.f} * xym;

		auto v0xy = math::vector4f{tri.v0.x, tri.v0.y, 0.f, 0.f};
		auto v1xy = math::vector4f{tri.v1.x, tri.v1.y, 0.f, 0.f};
		auto v2xy = math::vector4f{tri.v2.x, tri.v2.y, 0.f, 0.f};

		float de0xy = -dot_product(ne0xy, v0xy) + std::max(0.f, dp.x * ne0xy.x) + std::max(0.f, dp.y * ne0xy.y);
		float de1xy = -dot_product(ne1xy, v1xy) + std::max(0.f, dp.x * ne1xy.x) + std::max(0.f, dp.y * ne1xy.y);
		float de2xy = -dot_product(ne2xy, v2xy) + std::max(0.f, dp.x * ne2xy.x) + std::max(0.f, dp.y * ne2xy.y);

		auto pxy = vector4f(p.x, p.y, 0.f, 0.f);

		if ((dot_product(ne0xy, pxy) + de0xy) < 0.f || (dot_product(ne1xy, pxy) + de1xy) < 0.f || (dot_product(ne2xy, pxy) + de2xy) < 0.f)
			return false;


		// yz-plane projection overlap
		auto yzm = (n.x < 0.f ? -1.f : 1.f);
		vector4f ne0yz = vector4f{-tri.edge0().z, tri.edge0().y, 0.f, 0.f} * yzm;
		vector4f ne1yz = vector4f{-tri.edge1().z, tri.edge1().y, 0.f, 0.f} * yzm;
		vector4f ne2yz = vector4f{-tri.edge2().z, tri.edge2().y, 0.f, 0.f} * yzm;

		auto v0yz = math::vector4f{tri.v0.y, tri.v0.z, 0.f, 0.f};
		auto v1yz = math::vector4f{tri.v1.y, tri.v1.z, 0.f, 0.f};
		auto v2yz = math::vector4f{tri.v2.y, tri.v2.z, 0.f, 0.f};

		float de0yz = -dot_product(ne0yz, v0yz) + std::max(0.f, dp.y * ne0yz.x) + std::max(0.f, dp.z * ne0yz.y);
		float de1yz = -dot_product(ne1yz, v1yz) + std::max(0.f, dp.y * ne1yz.x) + std::max(0.f, dp.z * ne1yz.y);
		float de2yz = -dot_product(ne2yz, v2yz) + std::max(0.f, dp.y * ne2yz.x) + std::max(0.f, dp.z * ne2yz.y);

		auto pyz = vector4f(p.y, p.z, 0.f, 0.f);

		if ((dot_product(ne0yz, pyz) + de0yz) < 0.f || (dot_product(ne1yz, pyz) + de1yz) < 0.f || (dot_product(ne2yz, pyz) + de2yz) < 0.f)
			return false;


		// zx-plane projection overlap
		auto zxm = (n.y < 0.f ? -1.f : 1.f);
		vector4f ne0zx = vector4f{-tri.edge0().x, tri.edge0().z, 0.f, 0.f} * zxm;
		vector4f ne1zx = vector4f{-tri.edge1().x, tri.edge1().z, 0.f, 0.f} * zxm;
		vector4f ne2zx = vector4f{-tri.edge2().x, tri.edge2().z, 0.f, 0.f} * zxm;

		auto v0zx = math::vector4f{tri.v0.z, tri.v0.x, 0.f, 0.f};
		auto v1zx = math::vector4f{tri.v1.z, tri.v1.x, 0.f, 0.f};
		auto v2zx = math::vector4f{tri.v2.z, tri.v2.x, 0.f, 0.f};

		float de0zx = -dot_product(ne0zx, v0zx) + std::max(0.f, dp.y * ne0zx.x) + std::max(0.f, dp.z * ne0zx.y);
		float de1zx = -dot_product(ne1zx, v1zx) + std::max(0.f, dp.y * ne1zx.x) + std::max(0.f, dp.z * ne1zx.y);
		float de2zx = -dot_product(ne2zx, v2zx) + std::max(0.f, dp.y * ne2zx.x) + std::max(0.f, dp.z * ne2zx.y);

		auto pzx = vector4f(p.z, p.x, 0.f, 0.f);

		if ((dot_product(ne0zx, pzx) + de0zx) < 0.f || (dot_product(ne1zx, pzx) + de1zx) < 0.f || (dot_product(ne2zx, pzx) + de2zx) < 0.f)
			return false;


		return true;
	}


	inline auto intersect_aabc_triangle(aabc_t const& aabc, triangle_t const& tri) -> bool
	{
		return intersect_aabb_triangle(aabc, tri);
	}

} }

