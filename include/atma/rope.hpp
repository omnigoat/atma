#pragma once

#include <atma/utf/utf8_char.hpp>

#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/memory.hpp>

#include <variant>
#include <optional>
#include <array>


namespace atma::detail
{
	constexpr size_t const rope_branching_factor = 4;
	constexpr size_t const rope_buffer_size = 512;
	constexpr size_t const rope_minimum_split_size = rope_buffer_size / 3;

	struct rope_node_t;
	using rope_node_ptr = intrusive_ptr<rope_node_t>;
}



//
// text-info
//
namespace atma::detail
{
	struct text_info_t
	{
		uint32_t bytes = 0;
		uint32_t characters = 0;
		uint32_t line_breaks = 0;
		uint32_t pad = 0;

		static text_info_t from_str(char const* str, size_t sz)
		{
			ATMA_ASSERT(str);

			text_info_t r;
			for (auto x : utf8_const_span_t{str, sz})
			{
				r.bytes += (uint32_t)x.size_bytes();
				++r.characters;
				if (utf8_char_is_newline(x))
					++r.line_breaks;
			}

			return r;
		}
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

		rope_node_info_t(text_info_t const& info, rope_node_ptr const& node = rope_node_ptr::null)
			: text_info_t(info)
			, node(node)
		{}

		rope_node_ptr node;
	};

	inline auto operator + (rope_node_info_t const& lhs, text_info_t const& rhs) -> rope_node_info_t
	{
		return rope_node_info_t{(text_info_t&)lhs + rhs, lhs.node};
	}


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
		auto children_range() const -> decltype(auto) { return (children_); }

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
		std::array<rope_node_info_t, rope_branching_factor> children_;
	};

}


//
// rope_node_leaf_t
//
namespace atma::detail
{
	struct rope_node_leaf_t
	{
		template <typename... Args>
		rope_node_leaf_t(Args&&...);

		// this buffer can only ever be appended to. nodes store how many
		// characters/bytes they address inside this buffer, so we can append
		// more data to this buffer and maintain an immutable data-structure
		size_t size = 0;
		char buf[rope_buffer_size];
	};
}


//
//  rope_node_t
//
namespace atma::detail
{
	struct rope_node_t : atma::ref_counted_of<rope_node_t>
	{
		rope_node_t();

		rope_node_t(rope_node_internal_t const& x)
			: variant_(x)
		{}

		rope_node_t(rope_node_leaf_t const& x)
			: variant_(x)
		{}

		auto length() const -> size_t;
		auto find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>;

		template <typename F>
		auto edit_chunk_at_char(size_t char_idx, rope_node_info_t const&, F&&) -> rope_edit_result_t;

		auto known_internal() const -> rope_node_internal_t const*
		{
			return &std::get<rope_node_internal_t>(variant_);
		}

		auto known_leaf() -> rope_node_leaf_t&
		{
			return std::get<rope_node_leaf_t>(variant_);
		}

	private:
		template <typename... Args>
		auto visit(Args&&... args)
		{
			return std::visit(visit_with{std::forward<Args>(args)...}, variant_);
		}

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
	inline rope_node_t::rope_node_t()
		: variant_(rope_node_leaf_t())
	{}

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
	inline auto rope_node_t::edit_chunk_at_char(size_t char_idx, rope_node_info_t const& info, F&& f) -> rope_edit_result_t
	{
		return this->visit(
			[&, f = std::forward<F>(f)](rope_node_internal_t& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(char_idx);
				auto const& child = x.child_at(child_idx);

				// recurse
				auto [l_info, r_info] = child.node->edit_chunk_at_char(child_rel_idx, child, f);
				auto result = x.clone_with(child_idx, l_info, r_info);
				return result;
			},

			[&, f = std::forward<F>(f)](rope_node_leaf_t& x) -> rope_edit_result_t
			{
				//auto y = const_cast<rope_node_leaf_t&>(x);
				return std::invoke(f, char_idx, info, x.buf, x.size);
			}
		);
	}
}


//
// rope_node_leaf_t implementation
//
namespace atma::detail
{
	inline auto rope_node_leaf_construct_(size_t& buf_size, char* buf)
	{}

	template <typename... Args>
	inline auto rope_node_leaf_construct_(size_t& buf_size, char* buf, src_bounded_memxfer_t<char const> x, Args&&... args)
	{
		atma::memory::memcpy(xfer_dest(buf + buf_size), x);
		buf_size += x.size();

		rope_node_leaf_construct_(buf_size, buf, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline rope_node_leaf_t::rope_node_leaf_t(Args&&... args)
	{
		rope_node_leaf_construct_(size, buf, std::forward<Args>(args)...);
	}
}


//
// rope_node_internal_t implementation
//
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
	{
		for (size_t i = acc_count; i != rope_branching_factor; ++i)
			dest[i].node = rope_node_ptr::make();
	}

	template <typename... Args>
	inline rope_node_internal_t::rope_node_internal_t(Args&&... args)
	{
		rope_node_internal_construct_(child_count_, children_.data(), std::forward<Args>(args)...);
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

			if (child_count_ + 1 < rope_branching_factor)
			{
				auto sn = rope_node_ptr::make(rope_node_internal_t(
					xfer_src(children_, idx),
					l_info, r_info,
					xfer_src(children_, idx + 1, child_count_ - 1)));

				return rope_edit_result_t{
					rope_node_info_t{l_info + r_info, sn},
					rope_maybe_node_info_t{}};
			}
			else
			{
				// something something split
				auto left_size = rope_branching_factor / 2 + 1;
				auto right_size = rope_branching_factor / 2;

				rope_node_ptr ln, rn;

				if (idx < left_size)
				{
					ln = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, idx), l_info, r_info));
					rn = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, idx, child_count_ - idx - 1)));
				}
				if (idx + 1 < left_size)
				{
					ln = rope_node_ptr::make(rope_node_internal_t(xfer_src(children_, idx), l_info));
					rn = rope_node_ptr::make(rope_node_internal_t(r_info, xfer_src(children_, idx + 1, right_size)));
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
				xfer_src(children_, idx + 1, child_count_ - idx - 1)});

			return rope_edit_result_t{rope_node_info_t{l_info, s}, rope_maybe_node_info_t()};
		}
	}

	inline auto rope_node_internal_t::push(rope_node_info_t const& x) const -> rope_node_ptr
	{
		ATMA_ASSERT(child_count_ < rope_branching_factor);
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
			[str, sz](size_t char_idx, detail::rope_node_info_t const& info, char* buf, size_t& buf_size)
			{
				auto byte_idx = utf8_charseq_idx_to_byte_idx(buf, buf_size, char_idx);

				auto combined_bytes = info.bytes + sz;

				// we will never have to split this node
				if (combined_bytes <= detail::rope_buffer_size)
				{
					// todo: one loop for copy + info-calculation
					auto affix_string_info = detail::text_info_t::from_str(str, sz);

					// appending is possible
					if (byte_idx == info.bytes)
					{
						std::memcpy(buf + info.bytes, str, sz);
						buf_size += sz;
						auto new_info = info + affix_string_info;

						return detail::rope_edit_result_t{new_info, detail::rope_maybe_node_info_t{}};
					}
					// append is not possible, but the total new size is under
					// the minimum splittable size. like, we don't want two characters
					// here, two there. allocate one new leaf and insert it all
					else if (combined_bytes < detail::rope_minimum_split_size)
					{
						auto new_info = info + affix_string_info;
						new_info.node = detail::rope_node_ptr::make(
							detail::rope_node_leaf_t{
								xfer_src(buf, byte_idx),
								xfer_src(str, sz),
								xfer_src(buf + byte_idx, buf_size - byte_idx)});


						return detail::rope_edit_result_t{new_info, {}};
					}
				}

				// splitting is required
				{
					// we don't need a new node for lhs, just reference a subsequence
					auto lhs_info = info;
					lhs_info.characters = (uint32_t)char_idx;
					lhs_info.bytes = (uint32_t)byte_idx;

					detail::text_info_t rhs_info;
					rhs_info.bytes = (uint32_t)(info.bytes - byte_idx);
					rhs_info.characters = (uint32_t)(info.characters - char_idx);
					auto rhs_node = detail::rope_node_ptr::make(detail::rope_node_leaf_t{
						xfer_src(str, sz),
						xfer_src(buf + byte_idx, buf_size - byte_idx)});
					
					return detail::rope_edit_result_t{lhs_info, detail::rope_node_info_t{rhs_info, rhs_node}};
				}
			});

		if (rhs_info.has_value())
		{
			(detail::text_info_t&)root_ = lhs_info + *rhs_info;
			if (!lhs_info.node)
				lhs_info.node = root_.node;
			root_.node = detail::rope_node_ptr::make(detail::rope_node_internal_t{lhs_info, *rhs_info});
		}
		else if (lhs_info.node)
		{
			root_ = lhs_info;
		}
		else
		{
			(detail::text_info_t&)root_ = lhs_info;
		}
	}
}
