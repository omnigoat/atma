#pragma once

#include <atma/math/vector4f.hpp>

namespace atma { namespace math {

	struct aabb_t
	{
		aabb_t(vector4f const& origin, vector4f const& extents)
			: origin(origin), extents(extents)
		{}

		static auto from_minmax(vector4f const& min, vector4f const& max) -> aabb_t
		{
			return aabb_t{
				(min + max) / 2.f,
				(max - min) / 2.f};
		}

		auto origin() const -> vector4f const& { return origin_; }
		auto extents() const -> vector4f const& { return extents_; }
		auto volume() const -> float { return 8.f * extents.x * extents.y * extents.z; }

		auto inside(math::vector4f const& p) const -> bool
		{
			return !(
				p.x < origin_.x - extents_.x || origin_.x + extents_.x < p.x ||
				p.y < origin_.y - extents_.y || origin_.y + extents_.y < p.y ||
				p.z < origin_.z - extents_.z || origin_.z + extents_.z < p.z);
		}

		auto octant(math::vector4f const& p) const -> int
		{
			return (int)(origin_.x < p.x) + 2*(int)(origin_.y < p.y) + 4*(int)(origin_.z < p.z);
		}

	private:
		vector4f origin_;
		vector4f extents_;
	};

}}
