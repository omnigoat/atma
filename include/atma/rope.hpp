#pragma once

#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>

namespace atma::detail
{
	constexpr size_t const branching_factor = 4;
	constexpr size_t const character_chunk = 512;

	struct rope_storage_t
	{
		size_t size = 0;
		char data[character_chunk];
	};

	struct text_info_t
	{
		uint32_t bytes = 0;
		uint32_t characters = 0;
		uint32_t line_breaks = 0;
		uint32_t pad = 0;

		auto is_full() const -> bool { return bytes == character_chunk; }
	};

	struct rope_node_t;
	using rope_node_ptr = intrusive_ptr<rope_node_t>;

	struct rope_node_t : ref_counted
	{
		virtual auto length() const -> size_t = 0;

		virtual auto insert(size_t idx, char const*, size_t size) -> rope_node_ptr = 0;
	};


	struct rope_node_info_t : text_info_t
	{
		rope_node_ptr node;
	};

	struct rope_text_node_t : rope_node_t
	{

	};
}

namespace atma::detail
{

}

namespace atma::detail
{
	struct rope_internal_node_t;
	using  rope_internal_node_ptr = atma::intrusive_ptr<rope_internal_node_t>;

	struct rope_internal_node_t : rope_node_t
	{
		auto push(rope_node_info_t const&) const -> rope_node_ptr;

	private:
		rope_internal_node_t() = default;
		rope_internal_node_t(rope_node_info_t const* lhs, size_t lhs_size, rope_node_info_t const&);

	private: // rope_node_t implementation
		auto length() const -> size_t override;
		auto insert(size_t idx, char const*, size_t size) -> rope_node_ptr override;

	private: // utility
		auto children_range() const { return atma::pointer_range(children_, children_ + child_count_); }

		auto find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>;

	private:
		size_t child_count_ = 0;
		rope_node_info_t children_[branching_factor];

		friend enable_intrusive_ptr_make;
	};

	inline rope_internal_node_t::rope_internal_node_t(rope_node_info_t const* lhs, size_t lhs_size, rope_node_info_t const& rhs)
		: child_count_(lhs_size + 1)
	{
		ATMA_ASSERT(rhs.node);
		std::copy(lhs, lhs + lhs_size, children_);
		children_[lhs_size] = rhs;
	}

	inline auto rope_internal_node_t::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children_range())
			result += x.characters;
		return result;
	}

	inline auto rope_internal_node_t::push(rope_node_info_t const& x) const -> rope_node_ptr
	{
		ATMA_ASSERT(child_count_ < branching_factor);
		return rope_internal_node_ptr::make(children_, child_count_, x);
	}

	inline auto rope_internal_node_t::insert(size_t idx, char const* data, size_t size) -> rope_node_ptr
	{
		// find child
		auto [child_idx, child_rel_idx] = find_for_char_idx(idx);
		auto& child = children_[child_idx];

		// recurse
		auto node = child.node->insert(child_rel_idx, data, size);


		return shared_from_this<rope_internal_node_t>();
	}
	
	inline auto rope_internal_node_t::find_for_char_idx(size_t const idx) const -> std::tuple<size_t, size_t>
	{
		size_t child_idx = 0;
		size_t acc_chars = 0;
		for (auto const& child : children_range())
		{
			if (idx < acc_chars + child.characters)
				break;

			acc_chars += child.characters;
			++child_idx;
		}

		return std::make_tuple(child_idx, idx - acc_chars);
	}
}






namespace atma
{
	struct rope_t
	{
		rope_t();

		auto push_back(char const*, size_t) -> void;

	private:
		detail::rope_node_ptr root_;
	};

	inline rope_t::rope_t()
		: root_(detail::rope_internal_node_ptr::make())
	{}

	inline auto rope_t::push_back(char const* str, size_t sz) -> void
	{
		root_ = root_->insert(root_->length(), str, sz);
	}
}
