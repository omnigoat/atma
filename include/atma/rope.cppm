module;


#include <atma/utf/utf8_string.hpp>

#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/memory.hpp>
#include <atma/algorithm.hpp>

#include <variant>
#include <optional>
#include <array>
#include <functional>

export module atma.rope;

#define ATMA_ROPE_DEBUG_BUFFER 1

namespace atma::detail
{
	//constexpr size_t const rope_branching_factor = 4;
	//constexpr size_t const rope_buf_size = 512;
	//constexpr size_t const rope_buf_min_size = rope_buf_size / 3;

	// how many children an internal node has
	constexpr size_t const rope_branching_factor = 4;

	// rope-buffer size
	constexpr size_t const rope_buf_size = 9;

	// when we are making modifications to a buffer, there are several
	// breakpoints which determine our behaviour. those breakpoints
	// are as follows:
	//
	//  - rope_buf_edit_max_size
	//    ----------------------
	//    a maximum edit-size, which is the size of the buffer
	//    minus two bytes. this means if we're inserting a seam,
	//    we are guaranteed to be able to fit CRLF into the previous
	//    logical leaf-node. exceeding this requires splitting.
	//
	//  - rope_buf_edit_split_size
	//    ------------------------
	//    a size that when exceeded causes a split anyhow, which is
	//    used to prevent large memory allocations when many small
	//    edits are made in a row. edits made that result in a size
	//    smaller than this breakpoint result in the whole buffer
	//    being reallocated and updated
	//
	//  - rope_buf_edit_split_drift_size
	//    ------------------------------
	//    if we're editing (in the real-world case, inserting) a very
	//    short string into roughly the middle of the buffer, we'll
	//    allow the split point to drift a little so that we split
	//    after the inserted text. we're predicting that more insert-
	//    operations are going to happen afterwards (like someone
	//    typing).
	//
	constexpr size_t rope_buf_edit_max_size = rope_buf_size - 2;
	constexpr size_t rope_buf_edit_split_size = (rope_buf_size / 2) - (rope_buf_size / 32);
	constexpr size_t rope_buf_edit_split_drift_size = (rope_buf_size / 32);

	struct rope_node_t;
	struct rope_node_internal_t;
	struct rope_node_leaf_t;

	using rope_node_ptr = intrusive_ptr<rope_node_t>;
	using rope_node_internal = intrusive_ptr<rope_node_internal_t>;
	using rope_node_leaf_ptr = intrusive_ptr<rope_node_leaf_t>;

	namespace rope_utils
	{
		constexpr char CR = 0x0d;
		constexpr char LF = 0x0a;
	}

	using rope_dest_buf_t = dest_bounded_memxfer_t<char>;
	using rope_src_buf_t = src_bounded_memxfer_t<char const>;
}



//
//
//
#if 1
namespace atma::detail
{
	struct charbuf_t
	{
		using value_type = char;
		using allocator_type = std::allocator<char>;
		using tag_type = dest_memory_tag_t;

		static constexpr size_t Extent = rope_buf_size;

		bool empty() const { return size_ == 0; }
		size_t size() const { return size_; }
		size_t extent() const { return Extent; }

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

		char operator[](size_t idx) const { return chars_[idx]; }

	private:
		size_t size_ = 0;
		char chars_[Extent];
	};
}
#endif


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
// rope-node-info
//
namespace atma::detail
{
	struct rope_node_info_t : text_info_t
	{
		rope_node_info_t() = default;

		rope_node_info_t(rope_node_ptr const& node);

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
	struct rope_edit_result_t { rope_node_info_t left; rope_maybe_node_info_t right; bool seam = false; }; // std::tuple<rope_node_info_t, rope_maybe_node_info_t, bool>;
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
		charbuf_t buf;
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

		auto known_internal() -> rope_node_internal_t&
		{
			return std::get<rope_node_internal_t>(variant_);
		}

		auto known_leaf() -> rope_node_leaf_t&
		{
			return std::get<rope_node_leaf_t>(variant_);
		}

		auto calculate_text_info() const -> text_info_t;

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

		//template <typename F>
		//auto for_all_text(F&& f)
		//{
		//	this->visit([](rope_node_internal_t const& x){ for (auto& y : x.children_range()) if (y.node) y.node->for_all_text(f); },
		//		[](rope_node_leaf_t const& x) { if (x.bytes) f(xfer_src(x.buf, x.size))}
		//}

	private:
		std::variant<rope_node_internal_t, rope_node_leaf_t> variant_;
	};

	template <typename... Args>
	inline rope_node_ptr rope_make_leaf_ptr(Args&&... args)
	{
		return rope_node_ptr::make(rope_node_leaf_t{std::forward<Args>(args)...});
	}

	template <typename... Args>
	inline rope_node_ptr rope_make_internal_ptr(Args&&... args)
	{
		return rope_node_ptr::make(rope_node_internal_t{std::forward<Args>(args)...});
	}
}

namespace atma::detail
{
	template <typename F>
	inline auto rope_for_all_text(F f, rope_node_info_t const& ri)
	{
		if (!ri.node)
			return;

		ri.node->visit(
			[&f](rope_node_internal_t const& x)
			{ x.for_each_child(std::bind(&rope_for_all_text<F>, f, std::placeholders::_1)); }, //atma::curry(&rope_for_all_text<F>, f)); },
			[&ri, f = std::forward<F>(f)](rope_node_leaf_t const& x)
		{ std::invoke(f, ri); });
	}
}

// implementation
namespace atma::detail
{
	inline rope_node_info_t::rope_node_info_t(rope_node_ptr const& node)
		: text_info_t{node->calculate_text_info()}
		, node(node)
	{}
}



// something
namespace atma::detail
{
	inline auto is_break(rope_src_buf_t buf, size_t byte_idx) -> bool
	{
		ATMA_ASSERT(byte_idx < buf.size());

		return byte_idx == 0
			|| byte_idx == buf.size()
			|| (buf[byte_idx] >> 6 != !0b10) && (buf[byte_idx - 1] != 0x0d || buf[byte_idx] != 0x0a);
	}

	inline auto prev_break(rope_src_buf_t buf, size_t byte_idx) -> size_t
	{
		ATMA_ASSERT(byte_idx <= buf.size());

		if (byte_idx == 0)
			return 0;

		size_t i = byte_idx - 1;
		while (!is_break(buf, i))
			--i;
		return i;
	}

	inline auto next_break(rope_src_buf_t buf, size_t byte_idx) -> size_t
	{
		ATMA_ASSERT(byte_idx <= buf.size());

		if (byte_idx == buf.size())
			return buf.size();

		size_t i = byte_idx + 1;
		while (!is_break(buf, i))
			++i;
		return i;
	}

	inline auto find_split_point(rope_src_buf_t buf, size_t byte_idx) -> size_t
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

	inline auto insert_and_redistribute(text_info_t const& host, rope_src_buf_t hostbuf, rope_src_buf_t insbuf, size_t byte_idx)
		-> std::tuple<rope_node_info_t, rope_node_info_t>
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

		rope_node_info_t new_lhs, new_rhs;

		// inserted text is pre-split
		if (insbuf_split_idx == 0)
		{
			new_lhs = detail::rope_make_leaf_ptr(
				hostbuf.subspan(0, byte_idx),
				insbuf,
				hostbuf.subspan(byte_idx, split_idx - byte_idx - insbuf.size()));

			new_rhs = detail::rope_make_leaf_ptr(
				hostbuf.subspan(split_idx - insbuf.size(), hostbuf.size() - split_idx + insbuf.size()));
		}
		// inserted text is post-split
		else if (insbuf_split_idx == insbuf.size())
		{
			new_lhs = detail::rope_make_leaf_ptr(
				hostbuf.subspan(0, split_idx));

			new_rhs = detail::rope_make_leaf_ptr(
				hostbuf.subspan(split_idx, (byte_idx - split_idx)),
				insbuf,
				hostbuf.subspan(byte_idx, hostbuf.size() - byte_idx));
		}
		// inserted text is split in both halves
		else
		{
			new_lhs = detail::rope_make_leaf_ptr(
				hostbuf.subspan(0, split_idx - insbuf_split_idx),
				insbuf.subspan(0, insbuf_split_idx));

			new_rhs = detail::rope_make_leaf_ptr(
				insbuf.subspan(insbuf_split_idx, insbuf.size() - insbuf_split_idx),
				hostbuf.subspan(split_idx - insbuf_split_idx, hostbuf.size() - split_idx + insbuf_split_idx));
		}

		return std::make_tuple(
			rope_node_info_t{new_lhs},
			rope_node_info_t{new_rhs});
	}
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

	inline auto rope_node_t::find_for_char_idx(size_t idx) const -> std::tuple<size_t, size_t>
	{
		return this->visit(
			[idx](rope_node_internal_t const& x)
			{
				size_t child_idx = 0;
				size_t acc_chars = 0;
				for (auto const& child : x.children_range())
				{
					if (idx <= acc_chars + child.characters)
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

	inline auto rope_node_t::calculate_text_info() const -> text_info_t
	{
		return this->visit(
			[](rope_node_internal_t const& x)
			{
				return x.calculate_combined_info();
			},

			[](rope_node_leaf_t const& x)
			{
				return text_info_t::from_str(x.buf.data(), x.buf.size());
			});
	}

	inline auto fix_seam(size_t char_idx, detail::rope_node_info_t const& leaf_info, charbuf_t& buf)
	{
		//auto cr = detail::rope_src_buf_t("\015", 1);

		auto const new_buf_size = buf.size() + 1;

		// by definition, we should have enough space in any leaf-node to fix a seam
		ATMA_ASSERT(new_buf_size <= detail::rope_buf_size);

		// append if possible
		if (leaf_info.bytes == buf.size())
		{
			buf.push_back(rope_utils::LF);

			// don't increment linecount if we appended onto a CR character, giving CRLF
			auto new_leaf_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != rope_utils::CR)};

			return detail::rope_edit_result_t{new_leaf_info};
		}
		// buffer-size small enough to reallocate the node
		else if (new_buf_size < rope_buf_edit_split_size)
		{
			auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);

			auto new_text_info = text_info_t{
				leaf_info.bytes + 1,
				leaf_info.characters + 1,
				leaf_info.line_breaks + (buf[leaf_info.bytes] != rope_utils::CR)};

			auto new_node = detail::rope_make_leaf_ptr(
				xfer_src(buf, byte_idx),
				rope_src_buf_t("\n", 1),
				xfer_src(buf).skip(byte_idx));

			auto new_leaf_info = rope_node_info_t{new_text_info, new_node};

			return detail::rope_edit_result_t{new_leaf_info};
		}
		// split the node
		else
		{
			auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data(), buf.size(), char_idx);

			auto& leaf = leaf_info.node->known_leaf();

			auto [lhs_info, rhs_info] = insert_and_redistribute(leaf_info,
				xfer_src(leaf.buf),
				xfer_src(buf), byte_idx);

			//lhs_info.characters = (uint32_t)char_idx;
			//lhs_info.bytes = (uint32_t)byte_idx;
			//
			//rhs_info.bytes = (uint32_t)(leaf_info.bytes - byte_idx);
			//rhs_info.characters = (uint32_t)(leaf_info.characters - char_idx);
			auto rhs_node = detail::rope_make_leaf_ptr(
				xfer_src(buf, lhs_info.bytes, rhs_info.bytes));

			return detail::rope_edit_result_t{lhs_info, detail::rope_node_info_t{rhs_info, rhs_node}};
		}

	}


	template <typename F>
	inline auto rope_node_t::edit_chunk_at_char(size_t char_idx, rope_node_info_t const& info, F&& f) -> rope_edit_result_t
	{
		return this->visit(
			[&](rope_node_internal_t& x)
			{
				// find child
				auto [child_idx, child_rel_idx] = find_for_char_idx(char_idx);
				auto const& child = x.child_at(child_idx);

				// recurse
				auto [l_info, r_info, has_seam] = child.node->edit_chunk_at_char(child_rel_idx, child, f);
				auto result = x.clone_with(child_idx, l_info, r_info);

				// if we can fix a torn seam here, do so
				if (has_seam && child_idx > 0)
				{
					auto const& seamchild = x.child_at(child_idx - 1);
					seamchild.node->edit_chunk_at_char(seamchild.characters, seamchild, fix_seam);
				}
				// we can not, pass the seam up a level
				else
				{
					result.seam = has_seam;
				}

				return result;
			},

			[&, f = std::forward<F>(f)](rope_node_leaf_t& x) -> rope_edit_result_t
			{
				return std::invoke(f, char_idx, info, x.buf);
			}
			);
	}
}


//
// rope_node_leaf_t implementation
//
namespace atma::detail
{
	inline auto rope_node_leaf_construct_(charbuf_t& buf)
	{}

	template <typename... Args>
	inline auto rope_node_leaf_construct_(charbuf_t& buf, src_bounded_memxfer_t<char const> x, Args&&... args)
	{
		atma::memory_copy(xfer_dest(buf).skip(buf.size()), x);

		rope_node_leaf_construct_(buf, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline rope_node_leaf_t::rope_node_leaf_t(Args&&... args)
	{
#if ATMA_ROPE_DEBUG_BUFFER
		atma::memory_value_construct(atma::xfer_dest(buf), rope_buf_size);
#endif
		rope_node_leaf_construct_(buf, std::forward<Args>(args)...);
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
		atma::memory_copy_construct(
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
}

namespace atma::detail
{
	template <typename... Args>
	inline rope_node_internal_t::rope_node_internal_t(Args&&... args)
	{
		size_t acc = 0;
		rope_node_internal_construct_(acc, children_.data(), std::forward<Args>(args)...);
	}

	inline auto rope_node_internal_t::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children_range())
			result += x.characters;
		return result;
	}

	inline auto rope_node_internal_t::replaceable(size_t idx) const -> bool
	{
		return children_[idx].node->visit(
			[](rope_node_internal_t const&) { return false; },
			[&](rope_node_leaf_t const& x) { return children_[idx].bytes == 0; });
	}

	// clones 'this', but replaces child node at @idx with the node at @l_info, and inserts @maybe_r_info
	// if possible. if not it splits 'this' and returns an internal-node containing both nodes split from 'this'
	inline auto rope_node_internal_t::clone_with(size_t idx, rope_node_info_t const& l_info, std::optional<rope_node_info_t> const& maybe_r_info) const -> rope_edit_result_t
	{
		//ATMA_ASSERT(idx < child_count_);

		if (maybe_r_info.has_value())
		{
			auto const& r_info = *maybe_r_info;

			if (this->replaceable(idx + 1))
			{
				auto sn = rope_make_internal_ptr(
					xfer_src(children_, idx),
					l_info, r_info,
					xfer_src(children_, idx + 2, rope_branching_factor - 2));

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
					ln = rope_make_internal_ptr(xfer_src(children_, idx), l_info, r_info);
					rn = rope_make_internal_ptr(xfer_src(children_, idx, rope_branching_factor - idx - 1));
				}
				if (idx + 1 < left_size)
				{
					ln = rope_make_internal_ptr(xfer_src(children_, idx), l_info);
					rn = rope_make_internal_ptr(r_info, xfer_src(children_, idx + 1, right_size));
				}
				else
				{
					ln = rope_make_internal_ptr(xfer_src(children_, left_size));
					rn = rope_make_internal_ptr(l_info, r_info);
				}

				return rope_edit_result_t{
					rope_node_info_t{ln},
					rope_node_info_t{rn}};
			}
		}
		else
		{
			auto s = rope_make_internal_ptr(
				xfer_src(children_, idx),
				l_info,
				xfer_src(children_, idx + 1, rope_branching_factor - idx - 1));

			return rope_edit_result_t{rope_node_info_t{l_info, s}, rope_maybe_node_info_t()};
		}
	}

	inline auto rope_node_internal_t::calculate_combined_info() const -> text_info_t
	{
		return atma::foldl(children_range(), text_info_t(), functors::add);
	}

	inline auto rope_node_internal_t::push(rope_node_info_t const& x) const -> rope_node_ptr
	{
		//ATMA_ASSERT(child_count_ < rope_branching_factor);
		return rope_node_ptr(); ///rope_node_internal_ptr::make(children_, child_count_, x);
	}
}


namespace atma::detail
{
	inline auto rope_insert_(size_t char_idx, detail::rope_node_info_t const& leaf_info, charbuf_t& buf, detail::rope_src_buf_t src_text) -> detail::rope_edit_result_t
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
		bool can_fit_in_chunk = leaf_info.bytes + src.size() < detail::rope_buf_edit_max_size;

		// simple append is possible
		if (can_fit_in_chunk && byte_idx_is_at_end && buf_is_appendable)
		{
			// what we'd like to write:
			//    atma::memory_copy(xfer_dest_append(buf + leaf_info.bytes, buf_size), src);
			//
			// and buf_size would automatically be updated

			memory_copy(
				xfer_dest(buf).skip(leaf_info.bytes),
				src,
				std::size(src));

			//buf_size += src.size();

			auto affix_string_info = detail::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;
			return detail::rope_edit_result_t{result_info, {}, has_seam};
		}
		// append is wanted, but immutable data is in the way. reallocate & append
		else if (can_fit_in_chunk && byte_idx_is_at_end && !buf_is_appendable)
		{
			auto affix_string_info = detail::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = detail::rope_make_leaf_ptr(
				xfer_src(buf, leaf_info.bytes),
				src);

			return detail::rope_edit_result_t(result_info, {}, has_seam);
		}
		// we can fit everything into one chunk, but we are inserting into the middle
		else if (can_fit_in_chunk && !byte_idx_is_at_end)
		{
			auto affix_string_info = detail::text_info_t::from_str(src.data(), src.size());
			auto result_info = leaf_info + affix_string_info;

			result_info.node = detail::rope_make_leaf_ptr(
				xfer_src(buf, byte_idx),
				src,
				xfer_src(buf).skip(byte_idx));

			return detail::rope_edit_result_t(result_info, {}, has_seam);
		}

		// splitting is required
		{
			auto& leaf = leaf_info.node->known_leaf();

			auto [lhs_info, rhs_info] = detail::insert_and_redistribute(leaf_info, xfer_src(leaf.buf), src, byte_idx);

			return detail::rope_edit_result_t{lhs_info, rhs_info};
		}
	}
}





//
// rope_t
// --------
// 
//
export namespace atma
{
	struct rope_t
	{
		rope_t();

		auto push_back(char const*, size_t) -> void;
		auto insert(size_t char_idx, char const* str, size_t sz) -> void;

		friend std::ostream& operator << (std::ostream&, rope_t const&);

		template <typename F>
		auto for_all_text(F&& f) const;

	private:
		detail::rope_node_info_t root_;
	};

	rope_t::rope_t()
		: root_(detail::rope_make_internal_ptr())
	{}

	auto rope_t::push_back(char const* str, size_t sz) -> void
	{
		this->insert(root_.node->length(), str, sz);
	}

	auto rope_t::insert(size_t char_idx, char const* str, size_t sz) -> void
	{
		auto [lhs_info, rhs_info, seam] = root_.node->edit_chunk_at_char(char_idx, root_,
			[str, sz](size_t char_idx, detail::rope_node_info_t const& leaf_info, detail::charbuf_t& buf)
			{
				return detail::rope_insert_(char_idx, leaf_info, buf, xfer_src(str, sz));
			});

		if (rhs_info.has_value())
		{
			(detail::text_info_t&)root_ = lhs_info + *rhs_info;
			if (!lhs_info.node)
				lhs_info.node = root_.node;
			root_.node = detail::rope_make_internal_ptr(lhs_info, *rhs_info);
		}
		else
		{
			root_ = lhs_info;
		}
	}

	template <typename F>
	auto rope_t::for_all_text(F&& f) const
	{
		detail::rope_for_all_text(std::forward<F>(f), root_);
	}


	std::ostream& operator << (std::ostream& stream, rope_t const& x)
	{
		x.for_all_text(
			[&stream](detail::rope_node_info_t const& info)
			{
				stream << std::string_view(info.node->known_leaf().buf.data(), info.bytes);
			});

		return stream;
	}
}

