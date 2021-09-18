#if 1
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/bind.hpp>

#include <variant>
#include <optional>
#include <array>
#include <functional>
#include <utility>

#define ATMA_ROPE_DEBUG_BUFFER 1

import atma.types;
import atma.memory;


namespace atma::_rope_
{
	//constexpr size_t const branching_factor = 4;
	//constexpr size_t const buf_size = 512;
	//constexpr size_t const buf_min_size = buf_size / 3;

	// how many children an internal node has
	constexpr size_t const branching_factor = 4;

	// rope-buffer size
	constexpr size_t const buf_size = 9;

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
	constexpr size_t buf_edit_max_size = buf_size - 2;
	constexpr size_t buf_edit_split_size = (buf_size / 2) - (buf_size / 32);
	constexpr size_t buf_edit_split_drift_size = (buf_size / 32);

	template <typename> struct node_internal_t;
	template <typename> struct node_leaf_t;
	template <typename RT> using node_t = std::variant<node_internal_t<RT>, node_leaf_t<RT>>;

	template <typename RT> using node_internal_ptr = intrusive_ptr<node_internal_t<RT>>;
	template <typename RT> using node_leaf_ptr = intrusive_ptr<node_leaf_t<RT>>;
	template <typename RT> using node_ptr = std::shared_ptr<node_t<RT>>;

	namespace utils
	{
		constexpr char CR = 0x0d;
		constexpr char LF = 0x0a;
	}

	using dest_buf_t = dest_bounded_memxfer_t<char>;
	using src_buf_t = src_bounded_memxfer_t<char const>;
}


namespace atma
{
	struct rope_default_traits
	{
		// how many children an internal node has
		constexpr static size_t const branching_factor = 4;

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
	template <size_t Extent>
	struct charbuf_t
	{
		using value_type = char;
		using allocator_type = std::allocator<char>;
		using tag_type = dest_memory_tag_t;

		//static constexpr size_t Extent = RT::buf_size;

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
// text-info
//
namespace atma::_rope_
{
	struct text_info_t
	{
		uint32_t bytes = 0;
		uint32_t characters = 0;
		uint32_t line_breaks = 0;
		uint32_t pad = 0;

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
// node_info_t<RT>
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_info_t : text_info_t
	{
		node_info_t<RT>() = default;

		node_info_t<RT>(node_ptr<RT> const& node);

		node_info_t<RT>(text_info_t const& info, node_ptr<RT> const& node = node_ptr<RT>())
			: text_info_t(info)
			, node(node)
		{}

		node_ptr<RT> node;
	};

	template <typename RT>
	inline auto operator + (node_info_t<RT> const& lhs, text_info_t const& rhs) -> node_info_t<RT>
	{
		return node_info_t<RT>{(text_info_t&)lhs + rhs, lhs.node};
	}

	template <typename RT>
	using maybe_node_info_t = std::optional<node_info_t<RT>>;
}

//
// edit_result_t<RT>
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
		auto children_range() const -> decltype(auto) { return (children_); }

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
		std::array<node_info_t<RT>, branching_factor> children_;
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
//  node_t
//
namespace atma::_rope_
{
	template <typename RT>
	inline auto known_internal(node_ptr<RT> const& x) -> node_internal_t<RT>&
	{
		return std::get<node_internal_t<RT>>(*x);
	}

	template <typename RT>
	inline auto known_leaf(node_ptr<RT> const& x) -> node_leaf_t<RT>&
	{
		return std::get<node_leaf_t<RT>>(*x);
	}


	template <typename R, typename RT, typename... Args>
	inline auto visit_result(R const default_result, node_ptr<RT> const& x, Args&&... args) -> R
	{
		if (!x)
			return default_result;

		return std::visit(visit_with{std::forward<Args>(args)...}, *x);
	}

	template <typename RT, typename... Args>
	inline auto visit(node_ptr<RT> const& x, Args&&... args)
	{
		return std::visit(visit_with{std::forward<Args>(args)...}, *x);
	}

	template <typename RT>
	inline auto length(node_ptr<RT> const& x) -> size_t
	{
		return visit_result(size_t(), x,
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
		return visit(info.node,
			[&, f](node_internal_t<RT>& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(x, char_idx);
				auto const& child = x.child_at(child_idx);

				// recurse
				auto [l_info, r_info, has_seam] = edit_chunk_at_char(child, child_rel_idx, f);
				auto result = x.clone_with(child_idx, l_info, r_info);

				// if we can fix a torn seam here, do so
				if (has_seam && child_idx > 0)
				{
					auto const& seamchild = x.child_at(child_idx - 1);
					edit_chunk_at_char(seamchild, seamchild.characters, fix_seam<RT>);
				}
				// we can not, pass the seam up a level
				else
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
		return visit(node,
			[](node_internal_t<RT> const& x)
			{
				return x.calculate_combined_info();
			},

			[](node_leaf_t<RT> const& x)
			{
				return text_info_t::from_str(x.buf.data(), x.buf.size());
			});
	}

	template <typename RT, typename... Args>
	inline node_ptr<RT> make_internal_ptr(Args&&... args)
	{
		return std::make_shared<node_t<RT>>(node_internal_t<RT>{std::forward<Args>(args)...});
	}

	template <typename RT, typename... Args>
	inline node_ptr<RT> make_leaf_ptr(Args&&... args)
	{
		return std::make_shared<node_t<RT>>(node_leaf_t<RT>{std::forward<Args>(args)...});
	}
}

namespace atma::_rope_
{
	template <typename F, typename RT>
	inline auto for_all_text(F f, node_info_t<RT> const& ri)
	{
		if (!ri.node)
			return;

		visit(ri.node,
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

// implementation
namespace atma::_rope_
{
	template <typename RT>
	inline node_info_t<RT>::node_info_t(node_ptr<RT> const& node)
		: text_info_t{calculate_text_info(node)}
		, node(node)
	{}
}



// something
namespace atma::_rope_
{
	inline auto is_break(src_buf_t buf, size_t byte_idx) -> bool
	{
		ATMA_ASSERT(byte_idx < buf.size());

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

	inline auto find_split_point(src_buf_t buf, size_t byte_idx) -> size_t
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

// node_t implementation
namespace atma::_rope_
{
	template <typename RT>
	inline auto fix_seam(size_t char_idx, node_info_t<RT> const& leaf_info, charbuf_t<RT::buf_size>& buf)
	{
		//auto cr = _rope_::src_buf_t("\015", 1);

		auto const new_buf_size = buf.size() + 1;

		// by definition, we should have enough space in any leaf-node to fix a seam
		ATMA_ASSERT(new_buf_size <= _rope_::buf_size);

		// append if possible
		if (leaf_info.bytes == buf.size())
		{
			buf.push_back(utils::LF);

			// don't increment linecount if we appended onto a CR character, giving CRLF
			auto new_leaf_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != utils::CR)};

			return _rope_::edit_result_t<RT>{new_leaf_info};
		}
		// buffer-size small enough to reallocate the node
		else if (new_buf_size < buf_edit_split_size)
		{
			auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);

			auto new_text_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != utils::CR)};

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
		atma::memory_value_construct(atma::xfer_dest(buf), buf_size);
#endif

		(node_leaf_construct_(buf, std::forward<Args>(args)), ...);
	}
}


//
// node_internal_t implementation
//
namespace atma::_rope_
{
	template <typename RT, typename... Args>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, src_bounded_memxfer_t<node_info_t<RT> const> x, Args&&... args) -> void
	{
		atma::memory_copy_construct(
			xfer_dest(dest + acc_count),
			x);

		acc_count += x.size();
	}

	template <typename RT, typename... Args>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, node_info_t<RT> const& x) -> void
	{
		dest[acc_count] = x;
		++acc_count;
	}

	template <typename RT>
	template <typename... Args>
	inline node_internal_t<RT>::node_internal_t(Args&&... args)
	{
		size_t acc = 0;
		(node_internal_construct_(acc, children_.data(), std::forward<Args>(args)), ...);

		for (size_t i = acc; i != RT::branching_factor; ++i)
			children_[i].node = make_leaf_ptr<RT>();
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
		return visit(children_[idx].node,
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
					xfer_src(children_, idx + 2, branching_factor - 2));

				return edit_result_t<RT>{
					node_info_t<RT>{l_info + r_info, sn},
						maybe_node_info_t<RT>{}};
			}
			else
			{
				// something something split
				auto left_size = branching_factor / 2 + 1;
				auto right_size = branching_factor / 2;

				node_ptr<RT> ln, rn;

				if (idx < left_size)
				{
					ln = make_internal_ptr<RT>(xfer_src(children_, idx), l_info, r_info);
					rn = make_internal_ptr<RT>(xfer_src(children_, idx, branching_factor - idx - 1));
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
				xfer_src(children_, idx + 1, branching_factor - idx - 1));

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


namespace atma::_rope_
{
	template <typename RT>
	inline auto insert_(size_t char_idx, node_info_t<RT> const& leaf_info, charbuf_t<RT::buf_size>& buf, src_buf_t src_text) -> edit_result_t<RT>
	{
		ATMA_ASSERT(!src_text.empty());

		// determine if the LF character is at the front. if we
		// have that, then this is a "seam", and we want to insert
		// the LF character is the "previous" logical chunk, so
		// that if it tacks onto a CR character, we only count them
		// as one line-break
		bool has_seam = char_idx == 0 && src_text[0] == 0x0a;
		auto src = has_seam ? src_text.skip(1) : src_text;


		auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);
		//auto combined_bytes_size = leaf_info.bytes + src.size();

		// if we want to append, we must first make sure that the (immutable, remember) buffer
		// does not have any trailing information used by other trees. secondly, we must make
		// sure that the byte_idx of the character is actually the last byte_idx of the buffer
		bool buf_is_appendable = leaf_info.bytes == buf.size();
		bool byte_idx_is_at_end = leaf_info.bytes == byte_idx;
		bool can_fit_in_chunk = leaf_info.bytes + src.size() < _rope_::buf_edit_max_size;

		// simple append is possible
		if (can_fit_in_chunk && byte_idx_is_at_end && buf_is_appendable)
		{
			buf.append(src);

			auto affix_string_info = _rope_::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;
			return _rope_::edit_result_t<RT>{result_info, {}, has_seam};
		}
		// append is wanted, but immutable data is in the way. reallocate & append
		else if (can_fit_in_chunk && byte_idx_is_at_end && !buf_is_appendable)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, leaf_info.bytes),
				src);

			return _rope_::edit_result_t<RT>(result_info, {}, has_seam);
		}
		// we can fit everything into one chunk, but we are inserting into the middle
		else if (can_fit_in_chunk && !byte_idx_is_at_end)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = _rope_::make_leaf_ptr<RT>(
				xfer_src(buf, byte_idx),
				src,
				xfer_src(buf).skip(byte_idx));

			return _rope_::edit_result_t<RT>(result_info, {}, has_seam);
		}

		// splitting is required
		{
			auto& leaf = known_leaf(leaf_info.node);

			auto [lhs_info, rhs_info] = _rope_::insert_and_redistribute<RT>(leaf_info, xfer_src(leaf.buf), src, byte_idx);

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
		: root_(_rope_::make_internal_ptr<RT>())
	{}

	template <typename RT>
	auto basic_rope_t<RT>::push_back(char const* str, size_t sz) -> void
	{
		this->insert(_rope_::length(root_.node), str, sz);
	}

	template <typename RT>
	auto basic_rope_t<RT>::insert(size_t char_idx, char const* str, size_t sz) -> void
	{
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
				stream << std::string_view(std::get<_rope_::node_leaf_t<RT>>(*info.node).buf.data(), info.bytes);
			});

		return stream;
	}



	using rope_t = basic_rope_t<rope_default_traits>;
}
#endif

