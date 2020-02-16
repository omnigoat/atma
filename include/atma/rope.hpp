#pragma once

#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/memory.hpp>

#include <variant>
#include <optional>


namespace atma::detail
{
	constexpr size_t const branching_factor = 4;
	constexpr size_t const character_chunk = 512;

	struct rope_node_t;
	using rope_node_ptr = intrusive_ptr<rope_node_t>;
}


//
// node/text-info
//
namespace atma::detail
{
	struct text_info_t
	{
		uint32_t bytes = 0;
		uint32_t characters = 0;
		uint32_t line_breaks = 0;
		uint32_t pad = 0;

		auto is_full() const -> bool { return bytes == character_chunk; }
	};

	inline auto operator + (text_info_t const& lhs, text_info_t const& rhs) -> text_info_t
	{
		return text_info_t{
			lhs.bytes + rhs.bytes,
			lhs.characters + rhs.characters,
			lhs.line_breaks + rhs.line_breaks};
	}

	struct rope_node_info_t : text_info_t
	{
		rope_node_info_t() = default;

		rope_node_info_t(rope_node_ptr const& node)
			: node(node)
		{}

		rope_node_info_t(text_info_t const& info, rope_node_ptr const& node)
			: text_info_t(info)
			, node(node)
		{}

		rope_node_ptr node;
	};

	using rope_maybe_node_info_t = std::optional<rope_node_info_t>;
	using rope_edit_result_t = std::tuple<rope_node_info_t, rope_maybe_node_info_t>;
}




//
//  rope_node_internal_t
//
namespace atma::detail
{
	struct rope_node_internal_t
	{
		
		// accepts in-order arguments to fill children_
		template <typename... Args>
		rope_node_internal_t(Args&&...);

		auto length() const -> size_t;
		
		auto push(rope_node_info_t const&) const -> rope_node_ptr;
		auto child_at(size_t idx) const -> rope_node_info_t const& { return children_[idx]; }
		auto children_range() const { return atma::pointer_range(children_, children_ + child_count_); }

		auto clone_with(size_t idx, rope_node_info_t const&, std::optional<rope_node_info_t> const&) const -> rope_edit_result_t;


		auto calculate_combined_info() const -> text_info_t
		{
			text_info_t r;
			for (auto const& x : children_range())
				r = r + x;
			return r;
		}


	private:
		size_t child_count_ = 0;
		rope_node_info_t children_[branching_factor];
	};

}


//
namespace atma::detail
{
	struct rope_node_leaf_t
	{
		// this buffer can only ever be appended to. nodes store how many
		// characters/bytes they address inside this buffer, so we can append
		// more data to this buffer and maintain an immutable data-structure
		size_t size = 0;
		char buf[character_chunk];
	};
}


//
//  rope_node_t
//
namespace atma::detail
{
	struct rope_node_t : atma::ref_counted_of<rope_node_t>
	{
		rope_node_t() = default;

		rope_node_t(rope_node_internal_t const& x)
			: variant_(x)
		{}

		rope_node_t(rope_node_leaf_t const& x)
			: variant_(x)
		{}

		auto length() const -> size_t;
		auto find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>;

		template <typename F>
		auto edit_chunk_at_char(size_t char_idx, text_info_t const&, F&&) -> rope_edit_result_t;

		auto known_internal() const -> rope_node_internal_t const*
		{
			return &std::get<rope_node_internal_t>(variant_);
		}

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

	template <typename F>
	inline auto rope_node_t::edit_chunk_at_char(size_t char_idx, text_info_t const& info, F&& f) -> rope_edit_result_t
	{
		return this->visit(
			[&, f = std::forward<F>(f)](rope_node_internal_t const& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(char_idx);
				auto const& child = x.child_at(child_idx);

				// recurse
				auto [l_info, r_info] = child.node->edit_chunk_at_char(child_rel_idx, child, f);
				auto result = x.clone_with(child_idx, l_info, r_info);
				return result;
				//return rope_edit_result_t(rope_node_info_t{info, shared_from_this()}, std::optional<rope_node_info_t>{});
			},

			[&, f = std::forward<F>(f)](rope_node_leaf_t const& x)
			{
				return std::invoke(f, char_idx, info, x.buf);
			}
		);
	}
}

namespace atma::detail
{

}

namespace atma::detail
{
	template <typename... Args>
	inline auto rope_node_internal_construct_(size_t& acc_count, rope_node_info_t* dest, src_bounded_memxfer_t<rope_node_info_t const> x, Args&&... args) -> void
	{
		atma::memory::memcpy(
			xfer_dest(dest + acc_count),
			x);

		acc_count += x.size();

		rope_node_internal_construct_(acc_count, dest, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline auto rope_node_internal_construct_(size_t& acc_count, rope_node_info_t* dest, rope_node_info_t const& x, Args&&... args) -> void
	{
		dest[acc_count] = x;
		++acc_count;

		rope_node_internal_construct_(acc_count, dest, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline auto rope_node_internal_construct_(size_t& acc_count, rope_node_info_t* dest) -> void
	{}

	template <typename... Args>
	inline rope_node_internal_t::rope_node_internal_t(Args&&... args)
	{
		rope_node_internal_construct_(child_count_, children_, std::forward<Args>(args)...);
	}

	inline auto rope_node_internal_t::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children_range())
			result += x.characters;
		return result;
	}

	inline auto rope_node_internal_t::clone_with(size_t idx, rope_node_info_t const& l_info, std::optional<rope_node_info_t> const& maybe_r_info) const -> rope_edit_result_t
	{
		ATMA_ASSERT(idx < child_count_);

		if (maybe_r_info.has_value())
		{
			auto const& r_info = *maybe_r_info;

			if (child_count_ + 1 < branching_factor)
			{
				auto sn = rope_node_ptr::make(rope_node_internal_t(
					xfer_src(children_, idx),
					l_info, r_info,
					xfer_src(children_ + idx + 1, child_count_ - 1)));

				return rope_edit_result_t{
					rope_node_info_t{l_info + r_info, sn},
					rope_maybe_node_info_t{}};
			}
			else
			{
				// something something split
				auto left_size = branching_factor / 2 + 1;
				auto right_size = branching_factor / 2;

				rope_node_ptr ln, rn;

				if (idx < left_size)
				{
					ln = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, idx), l_info, r_info));
					rn = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_ + idx, child_count_ - idx - 1)));
				}
				if (idx + 1 < left_size)
				{
					ln = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, idx), l_info));
					rn = rope_node_ptr::make(rope_node_internal_t(r_info, xfer_src(children_ + idx + 1, right_size)));
				}
				else
				{
					ln = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, left_size)));
					rn = rope_node_ptr::make(rope_node_internal_t(l_info, r_info));
				}

				rope_node_info_t lni{ln->known_internal()->calculate_combined_info(), ln};
				rope_node_info_t rni{rn->known_internal()->calculate_combined_info(), rn};

				return rope_edit_result_t{lni, rni};
			}
		}
		else
		{
			auto s = rope_node_ptr::make(rope_node_internal_t{
				xfer_src(children_, idx), 
				l_info,
				xfer_src(children_ + idx + 1, child_count_ - idx - 1)});

			return rope_edit_result_t{rope_node_info_t{l_info, s}, rope_maybe_node_info_t()};
		}
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
		auto insert(size_t char_idx, char const* str, size_t sz) -> void;

	private:
		detail::rope_node_info_t root_;
	};

	inline rope_t::rope_t()
		: root_(detail::rope_node_ptr::make())
	{}

	inline auto rope_t::push_back(char const* str, size_t sz) -> void
	{
		this->insert(root_.node->length(), str, sz);
	}
	
	inline auto rope_t::insert(size_t char_idx, char const* str, size_t sz) -> void
	{
		auto [lhs_info, rhs_info] = root_.node->edit_chunk_at_char(char_idx, root_,
			[str, sz](size_t char_idx, detail::text_info_t const& info, char const* buf)
			{
				return detail::rope_edit_result_t{};
			});
	}
}
