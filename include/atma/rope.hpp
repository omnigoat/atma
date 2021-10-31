#if 1
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>

#include <variant>
#include <optional>
#include <array>
#include <functional>
#include <utility>
#include <span>
#include <concepts>

#define ATMA_ROPE_DEBUG_BUFFER 1

import atma.bind;
import atma.types;
import atma.memory;

// when we are making modifications to a buffer, there are several
// breakpoints which determine our behaviour. those breakpoints
// are as follows:
//
//  - buf_edit_max_size
//    ----------------------
//    a maximum edit-size, which is the size of the buffer
//    minus two bytes. this means if we're inserting a seam,
//    we are guaranteed to be able to fit CRLF into the previous
//    logical leaf-node. exceeding this requires splitting.
//
//  - buf_edit_split_size
//    ------------------------
//    a size that when exceeded causes a split anyhow, which is
//    used to prevent large memory allocations when many small
//    edits are made in a row. edits made that result in a size
//    smaller than this breakpoint result in the whole buffer
//    being reallocated and updated
//
//  - buf_edit_split_drift_size
//    ------------------------------
//    if we're editing (in the real-world case, inserting) a very
//    short string into roughly the middle of the buffer, we'll
//    allow the split point to drift a little so that we split
//    after the inserted text. we're predicting that more insert-
//    operations are going to happen afterwards (like someone
//    typing).
//



//
// forwards
//
namespace atma::_rope_
{
	struct text_info_t;
	template <typename> struct node_info_t;
	template <typename RT> using maybe_node_info_t = std::optional<node_info_t<RT>>;

	template <typename> struct node_internal_t;
	template <typename> struct node_leaf_t;
	template <typename> struct node_t;

	template <typename RT> using node_ptr = intrusive_ptr<node_t<RT>>;

	using dest_buf_t = dest_bounded_memxfer_t<char>;
	using src_buf_t = src_bounded_memxfer_t<char const>;

	namespace linebreaks
	{
		constexpr char CR = 0x0d;
		constexpr char LF = 0x0a;
	}
}


//
// traits
//
namespace atma
{
	struct rope_default_traits
	{
		// how many children an internal node has
		constexpr static size_t const branching_factor = 4;
		constexpr static size_t const minimum_branches = branching_factor / 2;

		// rope-buffer size
		constexpr static size_t const buf_size = 512;

		constexpr static size_t buf_edit_max_size = buf_size - 2;
		constexpr static size_t buf_edit_split_size = (buf_size / 2) - (buf_size / 32);
		constexpr static size_t buf_edit_split_drift_size = (buf_size / 32);
	};

	struct rope_test_traits
	{
		// how many children an internal node has
		constexpr static size_t const branching_factor = 4;
		constexpr static size_t const minimum_branches = branching_factor / 2;

		// rope-buffer size
		constexpr static size_t const buf_size = 9;

		constexpr static size_t buf_edit_max_size = buf_size - 2;
		constexpr static size_t buf_edit_split_size = (buf_size / 2) - (buf_size / 32);
		constexpr static size_t buf_edit_split_drift_size = (buf_size / 32);
	};
}


//
// charbuf_t
//
namespace atma::_rope_
{
	template <size_t ExtentX>
	struct charbuf_t
	{
		using value_type = char;
		using allocator_type = std::allocator<char>;
		using tag_type = dest_memory_tag_t;

		static constexpr size_t Extent = ExtentX;

		bool empty() const { return size_ == 0; }
		size_t size() const { return size_; }
		constexpr size_t extent() const { return Extent; }

		auto begin() -> value_type* { return chars_; }
		auto end() -> value_type* { return chars_ + size_; }
		auto begin() const -> value_type const* { return chars_; }
		auto end() const -> value_type const* { return chars_ + size_; }

		auto data() -> value_type* { return chars_; }
		auto data() const -> value_type const* { return chars_; }

		auto get_allocator() { return std::allocator<char>(); }

		void push_back(char x)
		{
			ATMA_ASSERT(size_ != Extent);
			chars_[size_++] = x;
		}

		void append(char const* data, size_t size)
		{
			ATMA_ASSERT(size_ + size <= Extent);
			memcpy(chars_ + size_, data, size);
			size_ += size;
		}

		void append(atma::bounded_memory_concept auto memory)
		{
			append(std::data(memory), std::size(memory));
		}

		char operator[](size_t idx) const { return chars_[idx]; }

	private:
		size_t size_ = 0;
		char chars_[Extent];
	};
}


//
// text_info_t
//
namespace atma::_rope_
{
	struct text_info_t
	{
		uint32_t bytes = 0;
		uint32_t characters = 0;
		uint32_t line_breaks = 0;

		auto can_append_at(size_t byte_idx) const { return byte_idx == bytes; }

		static text_info_t from_str(char const* str, size_t sz)
		{
			ATMA_ASSERT(str);

			text_info_t r;
			for (auto x : utf8_const_range_t{str, str + sz})
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
}


//
// node_info_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_info_t : text_info_t
	{
		node_info_t() = default;
		node_info_t(node_ptr<RT> const& node);
		node_info_t(text_info_t const& info, uint32_t children, node_ptr<RT> const&);
		node_info_t(text_info_t const& info, node_ptr<RT> const&);

		uint32_t children = 0;

		// weight = total number of leaf-nodes
		//uint16_t weight = 0;

		node_ptr<RT> node;
	};

	// the size of node_info_t should be the minimal size
	static_assert(sizeof(node_info_t<rope_default_traits>) == sizeof(node_ptr<rope_default_traits>) + sizeof(uint32_t) + sizeof(text_info_t),
		"we have an unexpected size for node_info_t");

	template <typename RT>
	inline auto operator + (node_info_t<RT> const& lhs, text_info_t const& rhs) -> node_info_t<RT>
	{
		return node_info_t<RT>{(text_info_t&)lhs + rhs, lhs.node};
	}
}


//
// edit_result_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct edit_result_t
	{
		node_info_t<RT> left;
		maybe_node_info_t<RT> right;
		bool seam = false;
	};
}


//
//  node_internal_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_internal_t
	{
		// accepts in-order arguments to fill children_
		template <typename... Args>
		node_internal_t(Args&&...);

		auto length() const -> size_t;

		auto push(node_info_t<RT> const&) const -> node_ptr<RT>;
		auto child_at(size_t idx) const -> node_info_t<RT> const& { return children_[idx]; }
		
		static constexpr size_t all_children = ~size_t();

		auto insert(size_t idx, node_info_t<RT> const& info)
		{
			ATMA_ASSERT(size_ != RT::branching_factor);
			ATMA_ASSERT(idx <= size_);

			for (int i = RT::branching_factor; i--> (idx + 1); )
			{
				children_[i] = children_[i - 1];
			}

			children_[idx] = info;
			++size_;
		}

		auto children_range(size_t limit = all_children) const
		{
			limit = (limit == all_children) ? size_ : limit;
			ATMA_ASSERT(limit <= RT::branching_factor);

			return std::span<node_info_t<RT> const>(children_.data(), limit);
		}

		auto empty_children_range() const
		{
			return std::span<node_info_t<RT> const>{children_.data() + size_, RT::branching_factor - size_};
		}

		auto children_size() const { return size_; }

		auto clone_with(size_t idx, node_info_t<RT> const&, std::optional<node_info_t<RT>> const&) const -> edit_result_t<RT>;

		auto replaceable(size_t idx) const -> bool;

		auto calculate_combined_info() const -> text_info_t;

		template <typename F>
		void for_each_child(F f) const
		{
			for (auto& x : children_)
			{
				if (!x.node)
					break;

				std::invoke(f, x);
			}
		}

	private:
		std::array<node_info_t<RT>, RT::branching_factor> children_;
		size_t size_ = 0;
	};

}


//
// node_leaf_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_leaf_t
	{
		template <typename... Args>
		node_leaf_t(Args&&...);

		// this buffer can only ever be appended to. nodes store how many
		// characters/bytes they address inside this buffer, so we can append
		// more data to this buffer and maintain an immutable data-structure
		charbuf_t<RT::buf_size> buf;
	};
}


//
// node_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_t : atma::ref_counted_of<node_t<RT>>
	{
		node_t() = default;
		node_t(node_internal_t<RT> const& x);
		node_t(node_leaf_t<RT> const& x);

		bool is_internal() const;
		bool is_leaf() const;

		auto known_internal() -> node_internal_t<RT>&;
		auto known_leaf() -> node_leaf_t<RT>&;

		// visit node with functors
		template <typename... Args>
		auto visit(Args&&... args);

	private:
		std::variant<node_internal_t<RT>, node_leaf_t<RT>> variant_;
	};


	template <typename RT, typename... Args>
	inline node_ptr<RT> make_internal_ptr(Args&&... args)
	{
		return node_ptr<RT>::make(node_internal_t<RT>{std::forward<Args>(args)...});
	}

	template <typename RT, typename... Args>
	inline node_ptr<RT> make_leaf_ptr(Args&&... args)
	{
		return node_ptr<RT>::make(node_leaf_t<RT>{std::forward<Args>(args)...});
	}
}

//
//  node algorithms
//
namespace atma::_rope_
{
	template <typename RT>
	inline auto length(node_ptr<RT> const& x) -> size_t
	{
		return visit(size_t(), x,
			[](node_internal_t<RT> const& x) { return x.length(); },
			[](node_leaf_t<RT> const& x) { return size_t(); });
	}

	// returns: <index-of-child-node, remaining-characters>
	template <typename RT>
	inline auto find_for_char_idx(node_internal_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		size_t child_idx = 0;
		size_t acc_chars = 0;
		for (auto const& child : x.children_range())
		{
			if (char_idx <= acc_chars + child.characters)
				break;

			acc_chars += child.characters;
			++child_idx;
		}

		return std::make_tuple(child_idx, char_idx - acc_chars);
	}

	template <typename RT, typename F>
	inline auto edit_chunk_at_char(node_info_t<RT> const& info, size_t char_idx, F f) -> edit_result_t<RT>
	{
		return info.node->visit(
			[&, f](node_internal_t<RT>& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(x, char_idx);
				auto const& child = x.child_at(child_idx);

				// if the node is null, something has gone irrevocably wrong
				ATMA_ASSERT(child.node);

				// recurse
				auto [l_info, r_info, has_seam] = edit_chunk_at_char(child, child_rel_idx, f);
				auto result = x.clone_with(child_idx, l_info, r_info);

				{
					result.seam = has_seam;
				}

				return result;
			},

			[&, f](node_leaf_t<RT>& x) -> edit_result_t<RT>
			{
				return std::invoke(f, char_idx, info, x.buf);
			});
	}

	template <typename RT>
	inline auto calculate_text_info(node_ptr<RT> const& node) -> text_info_t
	{
		return node->visit(
			[](node_internal_t<RT> const& x)
			{
				return x.calculate_combined_info();
			},

			[](node_leaf_t<RT> const& x)
			{
				return text_info_t::from_str(x.buf.data(), x.buf.size());
			});
	}

	template <typename F, typename RT>
	inline auto for_all_text(F f, node_info_t<RT> const& ri)
	{
		if (!ri.node)
			return;

		ri.node->visit(
			[f](node_internal_t<RT> const& x)
			{
				x.for_each_child(atma::curry(&for_all_text<F, RT>, f));
			},
			[&ri, f](node_leaf_t<RT> const& x)
			{
				std::invoke(f, ri);
			});
	}


}

// something
namespace atma::_rope_
{
	inline auto is_break(src_buf_t buf, size_t byte_idx) -> bool
	{
		ATMA_ASSERT(byte_idx <= buf.size());

		return byte_idx == 0
			|| byte_idx == buf.size()
			|| (buf[byte_idx] >> 6 != !0b10) && (buf[byte_idx - 1] != 0x0d || buf[byte_idx] != 0x0a);
	}

	inline auto prev_break(src_buf_t buf, size_t byte_idx) -> size_t
	{
		ATMA_ASSERT(byte_idx <= buf.size());

		if (byte_idx == 0)
			return 0;

		size_t i = byte_idx - 1;
		while (!is_break(buf, i))
			--i;
		return i;
	}

	inline auto next_break(src_buf_t buf, size_t byte_idx) -> size_t
	{
		ATMA_ASSERT(byte_idx <= buf.size());

		if (byte_idx == buf.size())
			return buf.size();

		size_t i = byte_idx + 1;
		while (!is_break(buf, i))
			++i;
		return i;
	}

	enum class split_bias
	{
		// always pick left
		hard_left,
		// prefer left
		left,
		// prefer right
		right,
		// always pick right
		hard_right,
	};

	inline auto find_split_point(src_buf_t buf, size_t byte_idx, split_bias bias = split_bias::right) -> size_t
	{
		size_t const left = is_break(buf, byte_idx)
			? byte_idx
			: prev_break(buf, byte_idx);

		size_t const right = next_break(buf, left);

		size_t const left_delta = byte_idx - left;
		size_t const right_delta = right - byte_idx;

		// negative number: pick left
		// positive number: pick right
		// zero: consult bias
		int const delta_diff = static_cast<int>(left_delta - right_delta);

		// we must split, so don't pick upon the boundaries, regardless
		// of what split_bias has said
		if (right == buf.size())
			return left; 
		else if (left == 0)
			return right;
		
		if (bias == split_bias::hard_left || delta_diff < 0)
			return left;
		else if (bias == split_bias::hard_right || 0 < delta_diff)
			return right;
		else if (bias == split_bias::left)
			return left;
		else
			return right;
	}

	inline auto find_internal_split_point(src_buf_t buf, size_t byte_idx) -> size_t
	{
		size_t left = (is_break(buf, byte_idx) && byte_idx != buf.size())
			? byte_idx
			: prev_break(buf, byte_idx);

		size_t right = next_break(buf, left);

		if (left == 0 || right != buf.size() && (byte_idx - left) >= (right - byte_idx))
			return right;
		else
			return left;
	}

	template <typename RT>
	inline auto insert_and_redistribute(text_info_t const& host, src_buf_t hostbuf, src_buf_t insbuf, size_t byte_idx)
		-> std::tuple<node_info_t<RT>, node_info_t<RT>>
	{
		ATMA_ASSERT(!insbuf.empty());
		ATMA_ASSERT(byte_idx < hostbuf.size());
		ATMA_ASSERT(utf8_byte_is_leading((byte const)hostbuf[byte_idx]));

		// determine split point
		size_t split_idx = 0;
		size_t insbuf_split_idx = 0;
		{
			constexpr auto splitbuf_size = size_t(8);
			constexpr auto splitbuf_halfsize = splitbuf_size / 2ull;
			static_assert(splitbuf_halfsize * 2 == splitbuf_size);

			char splitbuf[splitbuf_size] ={0};

			size_t result_size = hostbuf.size() + insbuf.size();
			size_t midpoint = result_size / 2;
			size_t ins_end_idx = byte_idx + insbuf.size();

			size_t bufcopy_start = midpoint - std::min(splitbuf_halfsize, midpoint);
			size_t bufcopy_end = std::min(result_size, midpoint + splitbuf_halfsize);

			for (size_t i = bufcopy_start; i != bufcopy_end; ++i)
				splitbuf[i - bufcopy_start]
				= (i < byte_idx) ? hostbuf[i]
				: (i < ins_end_idx) ? insbuf[i - byte_idx]
				: hostbuf[i - insbuf.size()]
				;

			split_idx = bufcopy_start + find_split_point(xfer_src(splitbuf, splitbuf_size), midpoint - bufcopy_start);

			insbuf_split_idx
				= (ins_end_idx <= split_idx) ? 0
				: (split_idx <= byte_idx) ? insbuf.size()
				: midpoint - byte_idx
				;

			ATMA_ASSERT(utf8_byte_is_leading((byte const)splitbuf[split_idx]));
		}

		node_info_t<RT> new_lhs, new_rhs;

		// inserted text is pre-split
		if (insbuf_split_idx == 0)
		{
			new_lhs = _rope_::make_leaf_ptr<RT>(
				hostbuf.subspan(0, byte_idx),
				insbuf,
				hostbuf.subspan(byte_idx, split_idx - byte_idx - insbuf.size()));

			new_rhs = _rope_::make_leaf_ptr<RT>(
				hostbuf.subspan(split_idx - insbuf.size(), hostbuf.size() - split_idx + insbuf.size()));
		}
		// inserted text is post-split
		else if (insbuf_split_idx == insbuf.size())
		{
			new_lhs = _rope_::make_leaf_ptr<RT>(
				hostbuf.subspan(0, split_idx));

			new_rhs = _rope_::make_leaf_ptr<RT>(
				hostbuf.subspan(split_idx, (byte_idx - split_idx)),
				insbuf,
				hostbuf.subspan(byte_idx, hostbuf.size() - byte_idx));
		}
		// inserted text is split in both halves
		else
		{
			new_lhs = _rope_::make_leaf_ptr<RT>(
				hostbuf.subspan(0, split_idx - insbuf_split_idx),
				insbuf.subspan(0, insbuf_split_idx));

			new_rhs = _rope_::make_leaf_ptr<RT>(
				insbuf.subspan(insbuf_split_idx, insbuf.size() - insbuf_split_idx),
				hostbuf.subspan(split_idx - insbuf_split_idx, hostbuf.size() - split_idx + insbuf_split_idx));
		}

		return std::make_tuple(
			node_info_t<RT>{new_lhs},
			node_info_t<RT>{new_rhs});
	}
}



namespace atma::_rope_
{
	template <typename RT>
	inline auto fix_seam(size_t char_idx, node_info_t<RT> const& leaf_info, charbuf_t<RT::buf_size>& buf)
	{
		//auto cr = _rope_::src_buf_t("\015", 1);

		auto const new_buf_size = buf.size() + 1;

		// by definition, we should have enough space in any leaf-node to fix a seam
		ATMA_ASSERT(new_buf_size <= RT::buf_size);

		// append if possible
		if (leaf_info.bytes == buf.size())
		{
			buf.push_back(linebreaks::LF);

			// don't increment linecount if we appended onto a CR character, giving CRLF
			auto new_leaf_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != linebreaks::CR)};

			return _rope_::edit_result_t<RT>{new_leaf_info};
		}
		// buffer-size small enough to reallocate the node
		else if (new_buf_size < RT::buf_edit_split_size)
		{
			auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);

			auto new_text_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != linebreaks::CR)};

			auto new_node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, byte_idx),
				src_buf_t("\n", 1),
				xfer_src(buf).skip(byte_idx));

			auto new_leaf_info = node_info_t<RT>{new_text_info, new_node};

			return _rope_::edit_result_t<RT>{new_leaf_info};
		}
		// split the node
		else
		{
			auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);

			auto& leaf = known_leaf(leaf_info.node);

			auto [lhs_info, rhs_info] = insert_and_redistribute<RT>(leaf_info,
				xfer_src(leaf.buf),
				xfer_src(buf), byte_idx);

			//lhs_info.characters = (uint32_t)char_idx;
			//lhs_info.bytes = (uint32_t)byte_idx;
			//
			//rhs_info.bytes = (uint32_t)(leaf_info.bytes - byte_idx);
			//rhs_info.characters = (uint32_t)(leaf_info.characters - char_idx);
			auto rhs_node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, lhs_info.bytes, rhs_info.bytes));

			return _rope_::edit_result_t<RT>{lhs_info, _rope_::node_info_t<RT>{rhs_info, rhs_node}};
		}

	}

}


// node_info_t implementation
namespace atma::_rope_
{
	template <typename RT>
	auto valid_children_count(node_ptr<RT> const& node) -> uint32_t
	{
		return !node ? 0 : node->visit(
				[](node_internal_t<RT> const& x)
				{
					return (uint32_t)x.children_range().size();
				},
				[](node_leaf_t<RT> const& x) { return 0u; });
	}

	template <typename RT>
	node_info_t<RT>::node_info_t(node_ptr<RT> const& node)
		: text_info_t{calculate_text_info(node)}
		, children(valid_children_count(node))
		, node(node)
	{}

	template <typename RT>
	node_info_t<RT>::node_info_t(text_info_t const& info, uint32_t children, node_ptr<RT> const& node)
		: text_info_t(info)
		, children(children)
		, node(node)
	{}

	template <typename RT>
	node_info_t<RT>::node_info_t(text_info_t const& info, node_ptr<RT> const& node)
		: text_info_t(info)
		, children(valid_children_count(node))
		, node(node)
	{}
}



//
// node_internal_t implementation
//
namespace atma::_rope_
{
	template <typename RT>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, range_of_element_type<node_info_t<RT>> auto&& xs) -> void
	{
		for (auto&& x : xs)
		{
			atma::memory_construct_at(xfer_dest(dest + acc_count), x);
			++acc_count;
		}
	}

	template <typename RT>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, node_info_t<RT> const& x) -> void
	{
		dest[acc_count] = x;
		++acc_count;
	}

	template <typename RT>
	template <typename... Args>
	inline node_internal_t<RT>::node_internal_t(Args&&... args)
	{
		(node_internal_construct_(size_, children_.data(), std::forward<Args>(args)), ...);
	}

	template <typename RT>
	inline auto node_internal_t<RT>::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children_range())
			result += x.characters;
		return result;
	}

	template <typename RT>
	inline auto node_internal_t<RT>::replaceable(size_t idx) const -> bool
	{
		return children_[idx].node->visit(
			[](node_internal_t<RT> const&) { return false; },
			[&](node_leaf_t<RT> const& x) { return children_[idx].bytes == 0; });
	}

	// clones 'this', but replaces child node at @idx with the node at @l_info, and inserts @maybe_r_info
	// if possible. if not it splits 'this' and returns an internal-node containing both nodes split from 'this'
	template <typename RT>
	inline auto node_internal_t<RT>::clone_with(size_t idx, node_info_t<RT> const& l_info, std::optional<node_info_t<RT>> const& maybe_r_info) const -> edit_result_t<RT>
	{
		//ATMA_ASSERT(idx < child_count_);

		if (maybe_r_info.has_value())
		{
			auto const& r_info = *maybe_r_info;

			if (this->replaceable(idx + 1))
			{
				auto sn = make_internal_ptr<RT>(
					xfer_src(children_, idx),
					l_info, r_info,
					xfer_src(children_, idx + 2, RT::branching_factor - 2));

				return edit_result_t<RT>{
					node_info_t<RT>{l_info + r_info, sn},
						maybe_node_info_t<RT>{}};
			}
			else
			{
				// something something split
				auto left_size = RT::branching_factor / 2 + 1;
				auto right_size = RT::branching_factor / 2;

				node_ptr<RT> ln, rn;

				if (idx < left_size)
				{
					ln = make_internal_ptr<RT>(xfer_src(children_, idx), l_info, r_info);
					rn = make_internal_ptr<RT>(xfer_src(children_, idx, RT::branching_factor - idx - 1));
				}
				if (idx + 1 < left_size)
				{
					ln = make_internal_ptr<RT>(xfer_src(children_, idx), l_info);
					rn = make_internal_ptr<RT>(r_info, xfer_src(children_, idx + 1, right_size));
				}
				else
				{
					ln = make_internal_ptr<RT>(xfer_src(children_, left_size));
					rn = make_internal_ptr<RT>(l_info, r_info);
				}

				return edit_result_t<RT>{
					node_info_t<RT>{ln},
						node_info_t<RT>{rn}};
			}
		}
		else
		{
			auto s = make_internal_ptr<RT>(
				xfer_src(children_, idx),
				l_info,
				xfer_src(children_, idx + 1, RT::branching_factor - idx - 1));

			return edit_result_t<RT>{node_info_t<RT>{l_info, s}, maybe_node_info_t<RT>()};
		}
	}

	template <typename RT>
	inline auto node_internal_t<RT>::calculate_combined_info() const -> text_info_t
	{
		return atma::foldl(children_range(), text_info_t(), functors::add);
	}

	template <typename RT>
	inline auto node_internal_t<RT>::push(node_info_t<RT> const& x) const -> node_ptr<RT>
	{
		//ATMA_ASSERT(child_count_ < branching_factor);
		return node_ptr<RT>(); ///node_internal_ptr::make(children_, child_count_, x);
	}
}

//
// node_leaf_t implementation
//
namespace atma::_rope_
{
	template <size_t Extent>
	inline auto node_leaf_construct_(charbuf_t<Extent>& buf)
	{}

	template <size_t Extent, typename... Args>
	inline auto node_leaf_construct_(charbuf_t<Extent>& buf, src_bounded_memxfer_t<char const> x)
	{
		buf.append(x);
	}

	template <typename RT>
	template <typename... Args>
	inline node_leaf_t<RT>::node_leaf_t(Args&&... args)
	{
#if ATMA_ROPE_DEBUG_BUFFER
		atma::memory_value_construct(atma::xfer_dest(buf, RT::buf_size));
#endif

		(node_leaf_construct_(buf, std::forward<Args>(args)), ...);
	}
}

//
// node_t implementation
//
namespace atma::_rope_
{
	template <typename RT>
	node_t<RT>::node_t(node_internal_t<RT> const& x)
		: variant_{x}
	{}

	template <typename RT>
	node_t<RT>::node_t(node_leaf_t<RT> const& x)
		: variant_{x}
	{}

	template <typename RT>
	bool node_t<RT>::is_internal() const
	{
		return variant_.index() == 0;
	}

	template <typename RT>
	bool node_t<RT>::is_leaf() const
	{
		return variant_.index() == 1;
	}

	template <typename RT>
	auto node_t<RT>::known_internal() -> node_internal_t<RT>&
	{
		return std::get<node_internal_t<RT>>(variant_);
	}

	template <typename RT>
	auto node_t<RT>::known_leaf() -> node_leaf_t<RT>&
	{
		return std::get<node_leaf_t<RT>>(variant_);
	}

	// visit node with functors
	template <typename RT>
	template <typename... Args>
	auto node_t<RT>::visit(Args&&... args)
	{
		return std::visit(visit_with{std::forward<Args>(args)...}, variant_);
	}
}



// algorithms
namespace atma::_rope_
{
	template <typename RT>
	struct insert_result_t
	{
		node_info_t<RT> lhs;
		maybe_node_info_t<RT> maybe_rhs;
	};

	template <typename RT>
	inline auto replace_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& repl_info) -> insert_result_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal());

		auto& dest_node = dest.node->known_internal();
		ATMA_ASSERT(idx < dest_node.children_size(), "you can't replace at this index");

		auto children = dest_node.children_range();

		auto result_node = make_internal_ptr<RT>(
			xfer_src(children, idx),
			repl_info,
			xfer_src(children, idx + 1, children.size() - idx - 1));

		auto result = node_info_t<RT>{dest, result_node};

		return insert_result_t<RT>{result};
	}

	template <typename RT>
	inline auto insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& ins_info) -> insert_result_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal());

		auto& dest_node = dest.node->known_internal();
		ATMA_ASSERT(idx <= dest.children, "you can't insert past the end of the dest-info child count");

		auto children = dest_node.children_range();
		bool insertion_is_at_back = idx == children.size();
		bool space_for_additional_child = children.size() < RT::branching_factor;

		//
		// the simplest case is where:
		//   a) the insertion index is at the back of children (we're appending)
		//   b) there's space left ^_^
		//
		// in this case we can simply append to the children of the node, as the
		// destination-info won't be updated and still address the original number
		// of children
		//
		if (insertion_is_at_back && space_for_additional_child)
		{
			dest_node.insert(idx, ins_info);
			node_info_t<RT> result{(dest + ins_info), dest.children + 1, dest.node};
			return {result};
		}
		//
		// the next-simplest case is when it's an insert in the middle. simply
		// recreate the node with the child in the middle.
		//
		else if (!insertion_is_at_back && space_for_additional_child)
		{
			auto result_node = make_internal_ptr<RT>(
				xfer_src(children, idx),
				ins_info,
				xfer_src(children, idx, children.size() - idx));

			node_info_t<RT> result{result_node};

			return {result};
		}
		//
		// the final case - we're out of space, so we must split the node
		//
		else
		{
			constexpr auto left_size = RT::branching_factor / 2 + 1;
			constexpr auto right_size = RT::branching_factor / 2;

			node_ptr<RT> ln, rn;

			if (idx < left_size)
			{
				ln = make_internal_ptr<RT>(
					xfer_src(children, idx),
					ins_info,
					xfer_src(children, idx, left_size - idx - 1));

				rn = make_internal_ptr<RT>(
					xfer_src(children.last(right_size)));
			}
			else
			{
				ln = make_internal_ptr<RT>(
					xfer_src(children, left_size));

				rn = make_internal_ptr<RT>(
					xfer_src(children, left_size, idx - left_size),
					ins_info,
					xfer_src(children, idx, children.size() - idx));
			}

			node_info_t<RT> lhs{ln};
			node_info_t<RT> rhs{rn};

			return insert_result_t<RT>{lhs, rhs};
		}
	}

	//
	// a very common operation involves a child node needing to be replaced, with a possible
	// additional sibling-node requiring insertion.
	//
	template <typename RT>
	inline auto replace_and_insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& repl_info, maybe_node_info_t<RT> const& maybe_ins_info) -> insert_result_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal());

		auto& dest_node = dest.node->known_internal();
		ATMA_ASSERT(idx < dest_node.children_size(), "index for replacement out of bounds");

		// in the simple case, this is just the replace operation
		if (!maybe_ins_info.has_value())
		{
			return replace_(dest, idx, repl_info);
		}

		auto& ins_info = maybe_ins_info.value();

		auto const children = dest_node.children_range();
		bool const space_for_additional_two_children = (children.size() + 1 <= RT::branching_factor);

		// we can fit the additional node in
		if (space_for_additional_two_children)
		{
			auto result_node = make_internal_ptr<RT>(
				xfer_src(children, idx),
				repl_info,
				ins_info,
				xfer_src(children, idx + 1, children.size() - idx - 1));
			
			node_info_t<RT> result{dest + ins_info, dest.children + 1, result_node};

			return {result};
		}
		// we need to split the node in two
		else
		{
			auto const left_size = RT::branching_factor / 2 + 1;
			auto const right_size = RT::branching_factor / 2;

			node_ptr<RT> ln, rn;

			// case 1: both nodes are to left of split
			if (idx + 1 < left_size)
			{
				ln = make_internal_ptr<RT>(
					xfer_src(children, idx),
					repl_info,
					ins_info,
					xfer_src(children, idx + 1, left_size - idx - 2));

				rn = make_internal_ptr<RT>(
					xfer_src(children, left_size - 1, right_size));
			}
			// case 2: nodes straddle split
			else if (idx < left_size)
			{
				ln = make_internal_ptr<RT>(
					xfer_src(children, idx),
					repl_info);

				rn = make_internal_ptr<RT>(
					ins_info,
					xfer_src(children, idx + 1, children.size() - idx - 1));
			}
			// case 3: nodes to the right of split
			else
			{
				ln = make_internal_ptr<RT>(
					xfer_src(children, left_size));

				rn = make_internal_ptr<RT>(
					xfer_src(children, left_size, (idx - left_size)),
					repl_info,
					ins_info,
					xfer_src(children, idx + 1, children.size() - idx - 1));
			}

			return insert_result_t<RT>{
				node_info_t<RT>{ln},
				node_info_t<RT>{rn}};
		}
	}


	//
	//
	//
#if 0
	template <typename RT>
	using node_info_stack_t = std::vector<node_info_t<RT>*>;

	constexpr struct _insert_node_
	{
		template <typename RT>
		auto operator ()(node_info_stack_t<RT>& stack, node_ptr<RT> dest, size_t idx, node_ptr<RT> ins) const -> node_info_t<RT>
		{
			node_info_t<RT> ins_info{ins};
			return this->operator ()(stack, dest, idx, ins_info);
		}

		template <typename RT>
		auto operator ()(node_info_stack_t<RT>& stack, node_ptr<RT> dest, size_t idx, node_info_t<RT> ins_info) const -> node_info_t<RT>
		{
			ATMA_ASSERT(!stack.empty());
			ATMA_ASSERT(dest->is_internal());
			//ATMA_ASSERT(ins->is_leaf());

			auto& desti = dest->known_internal();

			auto* info = stack.top();
			ATMA_ASSERT(info->node == dest);

			// no space to insert this node, we must split
			if (desti.empty_range().empty())
			{

			}

			return stack.pop();
		}

	private:
		//template <typename RT>
		//auto insert_(node_info_t<RT>& result, node_info_stack_t<RT>& stack, node_info_t<RT>& dest, node_ptr<RT> ins) const -> node_info_t<RT>
		//{
		//	return dest;
		//}

	} insert_node_;
#endif

	constexpr struct _append_node_
	{
	private:
		template <typename RT>
		auto append_node_(node_info_t<RT> const& dest, node_ptr<RT> const& ins) const -> insert_result_t<RT>
		{
			return dest.node->visit(
				[&](node_internal_t<RT> const& x)
				{
					auto const children = x.children_range();
					ATMA_ASSERT(children.size() >= RT::branching_factor / 2);

					// recurse if our right-most child is an internal node
					if (auto const& last = children[children.size() - 1]; last.node->is_internal())
					{
						auto [left, maybe_right] = append_node_(last, ins);
						return replace_and_insert_(dest, children.size() - 1, left, maybe_right);
					}
					// it's a leaf, so insert @ins into us
					else
					{
						return insert_(dest, children.size(), node_info_t<RT>{ins});
					}
				},
				[&](node_leaf_t<RT> const& x)
				{
					// the only way we can be here is if the root was a leaf node
					// we must root-split.
					return insert_result_t<RT>{dest, node_info_t<RT>{ins}};
				});
		}

	public:

		template <typename RT>
		auto operator ()(node_info_t<RT> const& dest, node_ptr<RT> x) const -> node_info_t<RT>
		{
			node_info_t<RT> result;

			if (dest.node == node_ptr<RT>::null)
			{
				return node_info_t<RT>{x};
			}
			else
			{
				auto const [lhs, rhs] = append_node_(dest, x);

				if (rhs.has_value())
				{
					return node_info_t<RT>{make_internal_ptr<RT>(lhs, *rhs)};
				}
				else
				{
					return lhs;
				}
			}
		}

	} append_node_;













	template <std::ranges::range R>
	auto to_vector(R&& r) {
		auto r_common = r | std::views::common;
		return std::vector(std::ranges::begin(r_common), r_common.end());
	}


	template <typename RT>
	void push_node_and_collapse_(std::vector<node_info_t<RT>>& stack, node_info_t<RT> const& x, bool leaf);

	template <typename RT>
	void collapse_(std::vector<node_info_t<RT>>& stack, bool leaf);

	template <typename RT>
	void push_node_and_collapse_(std::vector<node_info_t<RT>>& stack, node_info_t<RT> const& x, bool leaf)
	{
		stack.push_back(x);

		auto last_nodes_r = stack
			| std::ranges::views::reverse
			| std::ranges::views::take_while([leaf](node_info_t<RT> const& x) { return x.node->is_leaf() == leaf; })
			| std::ranges::views::reverse
			;

		if (std::ranges::size(last_nodes_r) >= 4)
		{
			collapse_(stack, leaf);
		}
	}



	template <typename RT>
	void collapse_(std::vector<node_info_t<RT>>& stack, bool leaf)
	{
		auto last_nodes_r = stack
			| std::ranges::views::reverse
			| std::ranges::views::take_while([leaf](node_info_t<RT> const& x) { return x.node->is_leaf() == leaf; })
			| std::ranges::views::reverse
			;

		ATMA_ASSERT(std::ranges::size(last_nodes_r) <= 4);

		{
			auto new_internal_node = make_internal_ptr<RT>(last_nodes_r);
			node_info_t<RT> new_internal_info{new_internal_node};

			stack.erase(std::begin(last_nodes_r).base().base(), stack.end());

			push_node_and_collapse_(stack, new_internal_info, false);
		}
	}

	template <typename RT>
	inline auto build_rope(src_buf_t str) -> node_info_t<RT>
	{
#if 1
		// remove null terminator if necessary
		if (str[str.size() - 1] == '\0')
			str = str.take(str.size() - 1);

		std::vector<node_info_t<RT>> stack;

		// case 1. the str is small enough to just be inserted as a leaf
		while (!str.empty())
		{
			size_t candidate_split_idx = std::min(str.size(), RT::buf_size);
			auto split_idx = find_split_point(str, candidate_split_idx, split_bias::hard_left);

			auto leaf_text = str.take(split_idx);
			str = str.skip(split_idx);

			auto new_leaf = node_info_t<RT>{make_leaf_ptr<RT>(leaf_text)};

			push_node_and_collapse_(stack, new_leaf, true);
		}

		collapse_(stack, true);
		collapse_(stack, false);
#endif

		return node_info_t<RT>{};
	}


	template <typename RT>
	inline auto build_rope_niave(src_buf_t str) -> node_info_t<RT>
	{
		// remove null terminator if necessary
		if (str[str.size() - 1] == '\0')
			str = str.take(str.size() - 1);

		node_info_t<RT> root;

		// case 1. the str is small enough to just be inserted as a leaf
		while (!str.empty())
		{
			size_t candidate_split_idx = std::min(str.size(), RT::buf_size);
			auto split_idx = find_split_point(str, candidate_split_idx, split_bias::hard_left);

			auto leaf_text = str.take(split_idx);
			str = str.skip(split_idx);

			if (root.node == node_ptr<RT>::null)
			{
				root = node_info_t{make_leaf_ptr<RT>(leaf_text)};
			}
			else
			{
				root = append_node_(root, make_leaf_ptr<RT>(leaf_text));
			}
		}

		return node_info_t<RT>{};
	}



	// returns std::tuple<bool, std::optional<R>> if all elements were equal, optional has the value



	constexpr struct _validate_rope_
	{
	private:
		using check_node_result_type = std::tuple<bool, uint>;

		template <typename RT>
		static auto check_node(node_info_t<RT> const& info, size_t min_children = RT::minimum_branches) -> check_node_result_type
		{
			ATMA_ASSERT(info.node);

			return info.node->visit(
				[&info, min_children](node_internal_t<RT> const& internal_node)
				{
					if (info.children < min_children)
						return std::make_tuple(false, uint());
					
					auto r = singular_result(internal_node.children_range(), atma::bind_from<1>(&check_node<RT>, RT::minimum_branches));

					if (r.has_value())
					{
						auto const& [good, depth] = r.value();
						return check_node_result_type{good, depth + 1};
					}
					else
					{
						// our children had different depths, return 1 as the depth to
						// indicate on which level things went wrong
						return check_node_result_type{false, 1};
					}
				},
				[](node_leaf_t<RT> const& leaf) -> check_node_result_type
				{
					return {true, 1};
				});
		}

	public:
		template <typename RT>
		auto operator ()(node_info_t<RT> const& x) const -> bool
		{
			// assume we're passed the root (hence '2' as the minimum branch)
			return std::get<0>(check_node(x, 2));
		}

	} validate_rope_;
	
















}








namespace atma::_rope_
{
	template <typename RT>
	inline auto insert_(size_t char_idx, node_info_t<RT> const& leaf_info, charbuf_t<RT::buf_size>& buf, src_buf_t insbuf) -> edit_result_t<RT>
	{
		ATMA_ASSERT(!insbuf.empty());

		// determine if the first character in our incoming text is an LF character,
		// and we're trying to insert this text at the front of this chunk. if we
		// are doing that, then we want to insert the LF character is the previous
		// logical chunk, so if it may tack onto any CR character at the end of that
		// previous chunk, resulting in us only counting them as one line-break
		bool inserting_at_front = char_idx == 0;
		bool lf_at_front = insbuf[0] == linebreaks::LF;
		bool has_seam = inserting_at_front && lf_at_front;
		if (has_seam)
		{
			insbuf = insbuf.skip(1);
		}

		auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);
		//auto combined_bytes_size = leaf_info.bytes + insbuf.size();

		// if we want to append, we must first make sure that the (immutable, remember) buffer
		// does not have any trailing information used by other trees. secondly, we must make
		// sure that the byte_idx of the character is actually the last byte_idx of the buffer
		bool buf_is_appendable = leaf_info.bytes == buf.size();
		bool byte_idx_is_at_end = leaf_info.bytes == byte_idx;
		bool can_fit_in_chunk = leaf_info.bytes + insbuf.size() < RT::buf_edit_max_size;

		// simple append is possible
		if (can_fit_in_chunk && byte_idx_is_at_end && buf_is_appendable)
		{
			buf.append(insbuf);

			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;
			return _rope_::edit_result_t<RT>{result_info, {}, has_seam};
		}
		// append is wanted, but immutable data is in the way. reallocate & append
		else if (can_fit_in_chunk && byte_idx_is_at_end && !buf_is_appendable)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, leaf_info.bytes),
				insbuf);

			return _rope_::edit_result_t<RT>(result_info, {}, has_seam);
		}
		// we can fit everything into one chunk, but we are inserting into the middle
		else if (can_fit_in_chunk && !byte_idx_is_at_end)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, byte_idx),
				insbuf,
				xfer_src(buf).skip(byte_idx));

			return _rope_::edit_result_t<RT>(result_info, {}, has_seam);
		}

		// splitting is required
		{
			auto& leaf = leaf_info.node->known_leaf();

			auto [lhs_info, rhs_info] = _rope_::insert_and_redistribute<RT>(leaf_info, xfer_src(leaf.buf), insbuf, byte_idx);

			return _rope_::edit_result_t<RT>{lhs_info, rhs_info};
		}
	}
}







namespace atma
{
	template <typename RopeTraits>
	struct basic_rope_t
	{
		basic_rope_t();

		auto push_back(char const*, size_t) -> void;
		auto insert(size_t char_idx, char const* str, size_t sz) -> void;

		template <typename RT2>
		friend std::ostream& operator << (std::ostream&, basic_rope_t<RT2> const&);

		template <typename F>
		auto for_all_text(F&& f) const
		{
			_rope_::for_all_text(std::forward<F>(f), root_);
		}

		decltype(auto) root() { return root_; }

	private:
		_rope_::node_info_t<RopeTraits> root_;
	};
}

namespace atma
{
	template <typename T>
	int fooey()
	{
		return 3;
	}
}



namespace atma
{
	template <typename RT>
	basic_rope_t<RT>::basic_rope_t()
		: root_(_rope_::make_leaf_ptr<RT>())
	{}

	template <typename RT>
	auto basic_rope_t<RT>::push_back(char const* str, size_t sz) -> void
	{
		this->insert(root_.characters, str, sz);
	}

	template <typename RT>
	auto basic_rope_t<RT>::insert(size_t char_idx, char const* str, size_t sz) -> void
	{
		ATMA_ASSERT(char_idx <= root_.characters);

		auto [lhs_info, rhs_info, seam] = edit_chunk_at_char(root_, char_idx,
			[str, sz](size_t char_idx, _rope_::node_info_t<RT> const& leaf_info, _rope_::charbuf_t<RT::buf_size>& buf)
			{
				return _rope_::insert_(char_idx, leaf_info, buf, xfer_src(str, sz));
			});

		if (rhs_info.has_value())
		{
			(_rope_::text_info_t&)root_ = lhs_info + *rhs_info;
			if (!lhs_info.node)
				lhs_info.node = root_.node;
			root_.node = _rope_::make_internal_ptr<RT>(lhs_info, *rhs_info);
		}
		else
		{
			root_ = lhs_info;
		}
	}

	template <typename RT>
	std::ostream& operator << (std::ostream& stream, basic_rope_t<RT> const& x)
	{
		x.for_all_text(
			[&stream](_rope_::node_info_t<RT> const& info)
			{
				stream << std::string_view(info.node->known_leaf().buf.data(), info.bytes);
			});

		return stream;
	}



	using rope_t = basic_rope_t<rope_default_traits>;
}
#endif

