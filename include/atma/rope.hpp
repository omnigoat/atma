#pragma once

#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>

#include <variant>



namespace atma::detail
{
	constexpr size_t const branching_factor = 4;
	constexpr size_t const character_chunk = 512;

	struct rope_node_internal_t;
	struct rope_node_leaf_t;
	struct rope_node_t;
	using rope_node_ptr = intrusive_ptr<rope_node_t>;
}


//
// internal structures
//
namespace atma::detail
{
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

	struct rope_node_info_t : text_info_t
	{
		rope_node_ptr node;
	};
}




//
//  rope_node_internal_t
//
namespace atma::detail
{
	struct rope_node_internal_t
	{
		auto push(rope_node_info_t const&) const -> rope_node_ptr;

		rope_node_internal_t() = default;
		rope_node_internal_t(rope_node_info_t const* lhs, size_t lhs_size, rope_node_info_t const&);

		auto length() const -> size_t;
		auto insert(size_t idx, char const*, size_t size) -> rope_node_ptr;

		auto child_at(size_t idx) const -> rope_node_info_t const& { return children_[idx]; }
		auto children_range() const { return atma::pointer_range(children_, children_ + child_count_); }
		auto find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>;

	private:
		size_t child_count_ = 0;
		rope_node_info_t children_[branching_factor];

		friend enable_intrusive_ptr_make;
	};
}


//
namespace atma::detail
{
	struct rope_node_leaf_t
	{

	};
}


//
//  rope_node_t
//
namespace atma::detail
{
	struct rope_node_t : atma::ref_counted_of<rope_node_t>
	{
		auto length() const -> size_t;
		auto find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>;
		auto insert(size_t char_idx, char const* str, size_t sz) -> rope_node_ptr;
	
	private:
		template <typename... Args>
		auto visit(Args&&... args) const
		{
			return std::visit(visit_with{std::forward<Args>(args)...}, variant_);
		}

		std::variant<rope_node_internal_t, rope_node_leaf_t> variant_;
	};

	
}


// rope_node_t implementation
namespace atma::detail
{
	inline auto rope_node_t::length() const -> size_t
	{
		return this->visit(
			[](rope_node_internal_t const& x) { return x.length(); },
			[](rope_node_leaf_t const& x) { return size_t(); });
	}

	inline auto atma::detail::rope_node_t::find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>
	{
		return this->visit(
			[idx](rope_node_internal_t const& x)
			{
				size_t child_idx = 0;
				size_t acc_chars = 0;
				for (auto const& child : x.children_range())
				{
					if (idx < acc_chars + child.characters)
						break;

					acc_chars += child.characters;
					++child_idx;
				}

				return std::make_tuple(child_idx, idx - acc_chars);
			},

			[idx](rope_node_leaf_t const& x)
			{
				return std::make_tuple(size_t(), size_t());
			});
	}

	inline auto rope_node_t::insert(size_t char_idx, char const* data, size_t sz) -> rope_node_ptr
	{
		return this->visit(
			[this, char_idx, data, sz](rope_node_internal_t const& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(char_idx);
				auto& child = x.child_at(child_idx);

				// recurse
				auto node = child.node->insert(child_rel_idx, data, sz);


				return shared_from_this();
			},

			[this, char_idx, data, sz](rope_node_leaf_t const& x)
			{
				return shared_from_this();
			}
		);
	}

}

namespace atma::detail
{

}

namespace atma::detail
{
	inline rope_node_internal_t::rope_node_internal_t(rope_node_info_t const* lhs, size_t lhs_size, rope_node_info_t const& rhs)
		: child_count_(lhs_size + 1)
	{
		ATMA_ASSERT(rhs.node);
		std::copy(lhs, lhs + lhs_size, children_);
		children_[lhs_size] = rhs;
	}

	inline auto rope_node_internal_t::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children_range())
			result += x.characters;
		return result;
	}

	inline auto rope_node_internal_t::push(rope_node_info_t const& x) const -> rope_node_ptr
	{
		ATMA_ASSERT(child_count_ < branching_factor);
		return rope_node_ptr(); ///rope_node_internal_ptr::make(children_, child_count_, x);
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
		: root_(detail::rope_node_ptr::make())
	{}

	inline auto rope_t::push_back(char const* str, size_t sz) -> void
	{
		root_ = root_->insert(root_->length(), str, sz);
	}
}
