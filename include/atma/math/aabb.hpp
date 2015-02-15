#pragma once

#include <atma/math/vector4f.hpp>

#include <array>


namespace atma { namespace math {

	struct from_cube_tag_t {};

	namespace { from_cube_tag_t const from_cube_tag; }


	struct aabb_t
	{
		aabb_t()
			: origin_{math::point4f(0.f, 0.f, 0.f)}
			, extents_{0.5f, 0.5f, 0.5f, 0.f}
		{}

		aabb_t(vector4f const& origin, vector4f const& extents)
			: origin_{origin}
			, extents_{extents}
		{}

		aabb_t(from_cube_tag_t, vector4f const& cube)
			: origin_{math::point4f(cube.x, cube.y, cube.z)}
			, extents_{cube.w, cube.w, cube.w, 0.f}
		{}

		static auto from_minmax(vector4f const& min, vector4f const& max) -> aabb_t
		{
			return aabb_t{
				(min + max) / 2.f,
				(max - min) / 2.f};
		}

		auto origin() const -> vector4f const& { return origin_; }
		auto extents() const -> vector4f const& { return extents_; }
		auto volume() const -> float { return 8.f * extents_.x * extents_.y * extents_.z; }

		auto compute_corners() const -> std::array<vector4f, 8>
		{
			return {
				origin_ + point4f(-extents_.x, -extents_.y, -extents_.z),
				origin_ + point4f( extents_.x, -extents_.y, -extents_.z),
				origin_ + point4f(-extents_.x,  extents_.y, -extents_.z),
				origin_ + point4f( extents_.x,  extents_.y, -extents_.z),
				origin_ + point4f(-extents_.x, -extents_.y,  extents_.z),
				origin_ + point4f( extents_.x, -extents_.y,  extents_.z),
				origin_ + point4f(-extents_.x,  extents_.y,  extents_.z),
				origin_ + point4f( extents_.x,  extents_.y,  extents_.z),
			};
		}

		auto octant(int idx) -> aabb_t
		{
			return aabb_t{
				math::point4f(
					(-0.5f + float((idx & 1))) * extents_.x + origin_.x,
					(-0.5f + float((idx & 2) >> 1)) * extents_.y + origin_.y,
					(-0.5f + float((idx & 4) >> 2)) * extents_.z + origin_.z),
				0.5f * extents_};
		}

		auto inside(math::vector4f const& p) const -> bool
		{
			return !(
				p.x < origin_.x - extents_.x || origin_.x + extents_.x < p.x ||
				p.y < origin_.y - extents_.y || origin_.y + extents_.y < p.y ||
				p.z < origin_.z - extents_.z || origin_.z + extents_.z < p.z);
		}

		auto octant_idx(math::vector4f const& p) const -> int
		{
			return (int)(origin_.x < p.x) + 2*(int)(origin_.y < p.y) + 4*(int)(origin_.z < p.z);
		}

	private:
		vector4f origin_;
		vector4f extents_;
	};

}}
