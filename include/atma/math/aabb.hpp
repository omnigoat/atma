#pragma once

#include <atma/math/vector4f.hpp>

#include <array>


namespace atma { namespace math {

	struct from_cube_tag_t {};

	namespace { from_cube_tag_t const from_cube_tag; }


	struct aabb_t
	{
		aabb_t()
			: center_{0.f, 0.f, 0.f, 1.f}
			, dims_{.5f, .5f, .5f, 0.f}
		{}

		aabb_t(vector4f const& center, vector4f const& dimensions)
			: center_{center}
			, dims_{dimensions}
		{}

		aabb_t(from_cube_tag_t, vector4f const& cube)
			: center_{math::point4f(cube.x, cube.y, cube.z)}
			, dims_{cube.w / 2.f, cube.w / 2.f, cube.w / 2.f, 0.f}
		{}

		static auto from_minmax(vector4f const& min, vector4f const& max) -> aabb_t
		{
			return aabb_t{
				(min + max) / 2.f,
				(max - min) / 2.f};
		}

		auto center() const       -> vector4f const& { return center_; }
		auto dimensions() const   -> vector4f const& { return dims_; }
		auto volume() const       -> float { return 8.f * dims_.x * dims_.y * dims_.z; }
		auto min_point() const    -> vector4f { return point4f(center_.x - dims_.x, center_.y - dims_.y, center_.z - dims_.z); }
		auto max_point() const    -> vector4f { return point4f(center_.x + dims_.x, center_.y + dims_.y, center_.z + dims_.z); }

		auto compute_corners() const -> std::array<vector4f, 8>
		{
			return {
				center_ + point4f(-dims_.x, -dims_.y, -dims_.z),
				center_ + point4f( dims_.x, -dims_.y, -dims_.z),
				center_ + point4f(-dims_.x,  dims_.y, -dims_.z),
				center_ + point4f( dims_.x,  dims_.y, -dims_.z),
				center_ + point4f(-dims_.x, -dims_.y,  dims_.z),
				center_ + point4f( dims_.x, -dims_.y,  dims_.z),
				center_ + point4f(-dims_.x,  dims_.y,  dims_.z),
				center_ + point4f( dims_.x,  dims_.y,  dims_.z),
			};
		}

		auto octant(int idx) -> aabb_t
		{
			return aabb_t{
				math::point4f(
					(-0.5f + float((idx & 1))) * dims_.x + center_.x,
					(-0.5f + float((idx & 2) >> 1)) * dims_.y + center_.y,
					(-0.5f + float((idx & 4) >> 2)) * dims_.z + center_.z),
				0.5f * dims_};
		}

		auto inside(math::vector4f const& p) const -> bool
		{
			return !(
				p.x < center_.x - dims_.x || center_.x + dims_.x < p.x ||
				p.y < center_.y - dims_.y || center_.y + dims_.y < p.y ||
				p.z < center_.z - dims_.z || center_.z + dims_.z < p.z);
		}

		auto octant_idx(math::vector4f const& p) const -> int
		{
			return (int)(center_.x < p.x) + 2*(int)(center_.y < p.y) + 4*(int)(center_.z < p.z);
		}

	private:
		vector4f center_;
		vector4f dims_;
	};

}}
