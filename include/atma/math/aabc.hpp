#pragma once

#include <atma/math/vector4f.hpp>

#include <array>


namespace atma { namespace math {

	struct aabc_t
	{
		aabc_t()
			: data_(0.f, 0.f, 0.f, 1.f)
		{}

		aabc_t(vector4f const& origin, float width)
			: data_(origin.x, origin.y, origin.z, width)
		{}

		aabc_t(float x, float y, float z, float w)
			: data_(x, y, z, w)
		{}

		auto origin() const -> vector4f { return point4f(data_.x, data_.y, data_.z); }
		auto radius() const -> float    { return data_.w; }
		auto volume() const -> float    { return data_.w * data_.w * data_.w; }

		auto compute_corners() const -> std::array<vector4f, 8>
		{
			auto origin = vector4f{data_.x, data_.y, data_.z, 1.f};

			return {
				origin + 0.5f * vector4f(-data_.w, -data_.w, -data_.w, 0.f),
				origin + 0.5f * vector4f( data_.w, -data_.w, -data_.w, 0.f),
				origin + 0.5f * vector4f(-data_.w,  data_.w, -data_.w, 0.f),
				origin + 0.5f * vector4f( data_.w,  data_.w, -data_.w, 0.f),
				origin + 0.5f * vector4f(-data_.w, -data_.w,  data_.w, 0.f),
				origin + 0.5f * vector4f( data_.w, -data_.w,  data_.w, 0.f),
				origin + 0.5f * vector4f(-data_.w,  data_.w,  data_.w, 0.f),
				origin + 0.5f * vector4f( data_.w,  data_.w,  data_.w, 0.f),
			};
		}

		auto octant(int idx) -> aabc_t
		{
			auto subr = 0.5f * data_.w;

			return aabc_t{
				math::point4f(
					(-0.5f + float((idx & 1) >> 0)) * subr + data_.x,
					(-0.5f + float((idx & 2) >> 1)) * subr + data_.y,
					(-0.5f + float((idx & 4) >> 2)) * subr + data_.z),
					subr};
		}

		auto inside(math::vector4f const& p) const -> bool
		{
			return !(
				p.x < data_.x - data_.w * 0.5f || data_.x + data_.w < p.x ||
				p.y < data_.y - data_.w * 0.5f || data_.y + data_.w < p.y ||
				p.z < data_.z - data_.w * 0.5f || data_.z + data_.w < p.z);
		}

		auto octant_idx(math::vector4f const& p) const -> int
		{
			return (int)(data_.x < p.x) + 2*(int)(data_.y < p.y) + 4*(int)(data_.z < p.z);
		}

	private:
		vector4f data_;
	};

}}
