#if 0
module;

#include <atma/assert.hpp>
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
#include <list>
#include <ranges>

#define ATMA_ROPE_DEBUG_BUFFER 1

export module atma.rope;

import atma.bind;
import atma.types;
import atma.memory;
import atma.intrusive_ptr;



//
// naming convention:
// 
// the namespace _rope_ is our rope-specific "detail" namespace
// 
// any "public" functions/types within _rope_ have regular naming conventions
// any functions/types within _rope_ that are "private" (should not be used
// outside of _rope_) are postfixed with an underscore
//










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
export namespace atma::_rope_
{
	struct text_info_t;
	template <typename> struct node_info_t;
	template <typename RT> using maybe_node_info_t = std::optional<node_info_t<RT>>;
	template <typename RT> using maybe_node_info_ref_t = std::optional<std::reference_wrapper<node_info_t<RT> const>>;

	template <typename> struct node_internal_t;
	template <typename> struct node_leaf_t;
	template <typename> struct node_t;

	template <typename RT> using node_ptr = intrusive_ptr<node_t<RT>>;
	template <typename RT> using node_internal_ptr = intrusive_ptr<node_internal_t<RT>>;
	template <typename RT> using node_leaf_ptr = intrusive_ptr<node_leaf_t<RT>>;

	using dest_buf_t = dest_bounded_memxfer_t<char>;
	using src_buf_t = src_bounded_memxfer_t<char const>;

	namespace charcodes
	{
		constexpr char null = 0x0;

		constexpr char cr = 0x0d;
		constexpr char lf = 0x0a;
	}

	template <typename T>
	constexpr auto ceil_div(T x, T y)
	{
		return (x + y - 1) / y;
	}

	template <typename> struct build_rope_t_;
}


//
// traits
//
export namespace atma
{
	template <size_t BranchingFactor, size_t BufferSize>
	struct rope_basic_traits
	{
		// how many children an internal node has
		constexpr static size_t const branching_factor = BranchingFactor;
		constexpr static size_t const minimum_branches = branching_factor / 2;

		// rope-buffer size
		constexpr static size_t const buf_size = BufferSize;

		constexpr static size_t buf_edit_max_size = buf_size - 2;
		constexpr static size_t buf_edit_split_size = (buf_size / 2) - (buf_size / 32);
		constexpr static size_t buf_edit_split_drift_size = (buf_size / 32);
	};

	using rope_default_traits = rope_basic_traits<4, 512>;
	using rope_test_traits = rope_basic_traits<4, 9>;
}


//
// charbuf_t
//
export namespace atma::_rope_
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

		auto back() const { return chars_[size_ - 1]; }
		auto front() const { return chars_[0]; }

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
export namespace atma::_rope_
{
	// dropped indicates how many characters at the front of the
	// buffer we're ignoring
	//
	// occasionally we'll have a bunch of LFs at the front of a chunk.
	// and then we'll append some text with a CR at the end of the previous
	// chunk. we want to keep the CRLF pair together, so we place an LF
	// character at the end of the previous chunk, and "drop" one character
	// from this chunk.
	struct text_info_t
	{
		uint16_t bytes = 0;
		uint16_t characters = 0;
		uint16_t dropped_bytes = 0;
		uint16_t dropped_characters = 0;
		uint16_t line_breaks = 0;
		uint16_t _pad_ = 0;

		auto all_bytes() const { return dropped_bytes + bytes; }
		auto all_characters() const { return dropped_characters + characters; }

		static text_info_t from_str(char const* str, size_t sz);
	};

	inline auto operator == (text_info_t const& lhs, text_info_t const& rhs) -> bool
	{
		return lhs.bytes == rhs.bytes
			&& lhs.characters == rhs.characters
			&& lhs.dropped_bytes == rhs.dropped_bytes
			&& lhs.dropped_characters == rhs.dropped_characters
			&& lhs.line_breaks == rhs.line_breaks;
	}

	inline auto operator + (text_info_t const& lhs, text_info_t const& rhs) -> text_info_t
	{
		return text_info_t{
			(uint16_t)(lhs.bytes + rhs.bytes),
			(uint16_t)(lhs.characters + rhs.characters),
			(uint16_t)(lhs.dropped_bytes + rhs.dropped_characters),
			(uint16_t)(lhs.dropped_characters + rhs.dropped_characters),
			(uint16_t)(lhs.line_breaks + rhs.line_breaks)};
	}

	inline auto operator - (text_info_t const& lhs, text_info_t const& rhs) -> text_info_t
	{
		return text_info_t{
			(uint16_t)(lhs.bytes - rhs.bytes),
			(uint16_t)(lhs.characters - rhs.characters),
			(uint16_t)(lhs.dropped_bytes - rhs.dropped_characters),
			(uint16_t)(lhs.dropped_characters - rhs.dropped_characters),
			(uint16_t)(lhs.line_breaks - rhs.line_breaks)};
	}
}


//
// node_info_t
//
export namespace atma::_rope_
{
	template <typename RT>
	struct node_info_t : text_info_t
	{
		node_info_t() = default;
		node_info_t(node_ptr<RT> const& node);
		node_info_t(text_info_t const& info, uint32_t children, node_ptr<RT> const&);
		node_info_t(text_info_t const& info, node_ptr<RT> const&);

		uint32_t children = 0;
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

	template <typename RT>
	inline auto operator - (node_info_t<RT> const& lhs, text_info_t const& rhs) -> node_info_t<RT>
	{
		return node_info_t<RT>{(text_info_t&)lhs - rhs, lhs.node};
	}
}


//
// edit_result_t
//
export namespace atma::_rope_
{
	template <typename RT>
	struct insert_result_t
	{
		node_info_t<RT> lhs;
		maybe_node_info_t<RT> maybe_rhs;
	};

	// SERIOUSLY, make edit_result_t derive from insert_result_t I guess

	enum class seam_t
	{
		none = 0,
		left = 1,
		right = 2
	};

	inline seam_t operator & (seam_t lhs, seam_t rhs) { return seam_t{static_cast<int>(lhs) & static_cast<int>(rhs)}; }
	inline seam_t operator | (seam_t lhs, seam_t rhs) { return seam_t{static_cast<int>(lhs) | static_cast<int>(rhs)}; }


	template <typename RT>
	struct edit_result_t
	{
		node_info_t<RT> left;
		maybe_node_info_t<RT> right;
		seam_t seam = seam_t::none;
	};
}


//
// node_t
//
export namespace atma::_rope_
{
	template <typename RT> struct node_t;
	template <typename RT> struct node_internal_t;
	template <typename RT> struct node_leaf_t;

	enum class node_type_t : uint32_t
	{
		branch,
		leaf,
	};


	template <typename RT>
	struct node_t : atma::ref_counted_of<node_t<RT>>
	{
		node_t(node_type_t, uint32_t height);

		constexpr bool is_internal() const;
		constexpr bool is_leaf() const;

		auto as_branch() -> node_internal_t<RT>&;
		auto as_leaf() -> node_leaf_t<RT>&;

		auto as_branch() const -> node_internal_t<RT> const&;
		auto as_leaf() const -> node_leaf_t<RT> const&;

		auto height() const -> uint32_t;

		// visit node with functors
		template <typename... Args>
		auto visit(Args&&... args);

		template <typename... Args>
		auto visit(Args&&... args) const;

	private:
		node_type_t const node_type_ : 8;
		uint32_t const height_ : 24;
	};

	template <auto lhs, auto rhs>
	struct is_same_value_t : std::false_type { static_assert(lhs == rhs, "oh dear"); };

	template <auto both>
	struct is_same_value_t<both, both> : std::true_type {};

	template <auto lhs, auto rhs>
	constexpr inline bool is_same_value_v = is_same_value_t<lhs, rhs>::value;

	static_assert(is_same_value_v<
		sizeof node_t<rope_test_traits>, 
		sizeof atma::ref_counted_of<node_t<rope_test_traits>> + sizeof(uint32_t)>,
		"node_t should be 64 bits big");
}



//
//  node_internal_t
//
export namespace atma::_rope_
{
	template <typename RT>
	struct node_internal_t : node_t<RT>
	{
		using children_type = std::span<node_info_t<RT> const>;
		using size_type = uint32_t;

		// accepts in-order arguments to fill children_
		template <typename... Args>
		node_internal_t(uint32_t height, Args&&...);

		auto length() const -> size_t;

		auto child_at(size_type idx) const -> node_info_t<RT> const&;
		auto child_at(size_type idx) -> node_info_t<RT>&;
		
		auto try_child_at(size_type idx) -> maybe_node_info_t<RT>;

		static constexpr size_t all_children = ~size_t();

		auto append(node_info_t<RT> const& info);

		auto children(size_t limit = all_children) const;

		auto calculate_combined_info() const -> text_info_t;

		template <typename F>
		void for_each_child(F f) const;

		auto has_space_for_another_child() const
		{
			return size_ < RT::branching_factor;
		}

	private:
		uint32_t size_ = 0;
		std::array<node_info_t<RT>, RT::branching_factor> children_;
	};

}


//
// node_leaf_t
//
export namespace atma::_rope_
{
	template <typename RT>
	struct node_leaf_t : node_t<RT>
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
// make_internal_ptr / make_leaf_ptr
//
export namespace atma::_rope_
{
	template <typename RT, typename... Args>
	inline node_ptr<RT> make_internal_ptr(Args&&... args)
	{
		return node_internal_ptr<RT>::make(std::forward<Args>(args)...);
	}

	template <typename RT, typename... Args>
	inline node_ptr<RT> make_leaf_ptr(Args&&... args)
	{
		return node_leaf_ptr<RT>::make(std::forward<Args>(args)...);
	}
}



//---------------------------------------------------------------------
//
//  trees
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT> struct tree_t;
	template <typename RT> struct tree_branch_t;
	template <typename RT> struct tree_leaf_t;

	template <typename RT>
	struct tree_base_t : node_info_t<RT>
	{
		using node_info_t<RT>::node_info_t;

		tree_base_t(node_info_t<RT> const& x)
			: node_info_t<RT>{x}
		{}

	private:
		using text_info_t::bytes;
		using text_info_t::characters;
		using text_info_t::dropped_bytes;
		using text_info_t::dropped_characters;
		using text_info_t::line_breaks;
		using text_info_t::_pad_;

		using node_info_t<RT>::children;
		using node_info_t<RT>::node;
	};

	template <typename RT>
	struct tree_t : tree_base_t<RT>
	{
		tree_t(node_info_t<RT> const& info)
			: tree_base_t<RT>{info}
		{}

		tree_t(node_ptr<RT> const& x)
			: tree_base_t<RT>{x}
		{}

		auto info() const -> node_info_t<RT> const& { return static_cast<node_info_t<RT> const&>(*this); }
		auto node() const -> node_t<RT>& { return *this->node_info_t<RT>::node; }

		auto height() const { return this->node().height(); }
		auto size() const -> size_t { return this->info().characters - this->info().dropped_characters; }
		auto size_bytes() const -> size_t { return this->info().bytes - this->info().dropped_bytes; }

		auto is_saturated() const { return info().children >= RT::branching_factor / 2; }

		// children as viewed by info
		auto children() const
		{
			return node().visit(
				[&](node_internal_t<RT> const& x) -> node_internal_t<RT>::children_type { return x.children(info().children); },
				[](node_leaf_t<RT> const&) -> node_internal_t<RT>::children_type { return typename node_internal_t<RT>::children_type{}; });
		}

		auto backing_children() const
		{
			return node().visit(
				[](node_internal_t<RT> const& x) { return x.children(); },
				[](node_leaf_t<RT> const&) { return typename node_internal_t<RT>::children_type{}; });
		}

		auto as_branch() const -> tree_branch_t<RT> const& { return static_cast<tree_branch_t<RT> const&>(*this); }
		auto as_leaf() const -> tree_leaf_t<RT> const& { return static_cast<tree_leaf_t<RT> const&>(*this); }
	};




	template <typename RT>
	struct tree_branch_t : tree_t<RT>
	{
		using tree_t<RT>::tree_t;

		auto node() const -> node_internal_t<RT>& { return static_cast<node_internal_t<RT>&>(this->tree_t<RT>::node()); }
		
		// children as viewed by info
		auto children() const
		{
			return this->node().children(this->info().children);
		}

		auto child_at(int idx) const -> node_info_t<RT> const& { return this->node().children()[idx]; }

		auto backing_children() const
		{
			return this->node().children();
		}

		auto append(node_info_t<RT> const& x)
		{
			return this->node().append(x);
		}
	};

}

//---------------------------------------------------------------------
//
//  tree_leaf_t
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT>
	struct tree_leaf_t : tree_t<RT>
	{
		using tree_t<RT>::tree_t;

		operator tree_t<RT>& () { return *static_cast<tree_t<RT>*>(this); }
		operator tree_t<RT> const& () const { return *static_cast<tree_t<RT> const*>(this); }

		auto node() const -> node_leaf_t<RT>& { return static_cast<node_leaf_t<RT>&>(this->tree_t<RT>::node()); }

		auto height() const { return 1; }

		auto data() const -> src_buf_t { return xfer_src(this->node().buf.data() + this->info().dropped_bytes, this->info().bytes); }

		auto byte_idx_from_char_idx(size_t char_idx) const -> size_t
		{
			return utf8_charseq_idx_to_byte_idx(this->node().buf.data() + this->info().dropped_bytes, this->info().bytes, char_idx);
		}
	};
}





//---------------------------------------------------------------------
//
//  rope
//
//---------------------------------------------------------------------
export namespace atma
{
	template <typename RopeTraits>
	struct basic_rope_t
	{
		basic_rope_t();

		auto push_back(char const*, size_t) -> void;
		auto insert(size_t char_idx, char const* str, size_t sz) -> void;

		auto split(size_t char_idx) const -> std::tuple<basic_rope_t<RopeTraits>, basic_rope_t<RopeTraits>>;

		template <typename F>
		auto for_all_text(F&& f) const;

		decltype(auto) root() const { return (root_); }

		// size in characters
		auto size() const -> size_t { return root_.characters; }
		auto size_byte() const -> size_t { return root_.bytes - root_.dropped_bytes; }

		auto operator == (std::string_view) const -> bool;
		

	private:
		basic_rope_t(_rope_::node_info_t<RopeTraits> const&);

	private:
		_rope_::node_info_t<RopeTraits> root_;

		friend struct _rope_::build_rope_t_<RopeTraits>;
	};

	template <typename RopeTraits>
	auto operator == (basic_rope_t<RopeTraits> const&, basic_rope_t<RopeTraits> const&) -> bool;
}


export namespace atma
{
	template <typename RT>
	struct basic_rope_char_iterator_t
	{
		basic_rope_char_iterator_t(basic_rope_t<RT> const& rope);

		auto operator ++() -> basic_rope_char_iterator_t&;
		auto operator  *() -> utf8_char_t;

	private:
		basic_rope_t<RT> const& rope_;
		_rope_::node_leaf_ptr<RT> leaf_;
		size_t leaf_characters_ = 0;
		size_t idx_ = 0;
		size_t rel_idx_ = 0;
	};
	
}

namespace atma::_rope_
{
	template <typename RT>
	struct basic_leaf_iterator_t
	{
		basic_leaf_iterator_t() = default;
		basic_leaf_iterator_t(basic_rope_t<RT> const& rope);

		auto operator ++() -> basic_leaf_iterator_t<RT>&;
		auto operator  *() -> tree_leaf_t<RT>;

	private:
		atma::vector<std::tuple<tree_branch_t<RT>, int>> tree_stack_;

		template <typename RT2>
		friend inline auto operator == (basic_leaf_iterator_t<RT2> const& lhs, basic_leaf_iterator_t<RT2> const& rhs) -> bool
		{
			return std::ranges::equal(lhs.tree_stack_, rhs.tree_stack_);
		}
	};

#if 0
	template <typename RT>
	inline auto operator == (basic_leaf_iterator_t<RT> const& lhs, basic_leaf_iterator_t<RT> const& rhs) -> bool
	{
		return std::ranges::equal(lhs.tree_stack_, rhs.tree_stack_);

		//auto lhsi = tree_stack_.begin();
		//auto rhsi = rhs.tree_stack_.begin();
		//
		//for (;;)
		//{
		//	if (*lhsi != *rhsi)
		//		return false;
		//}
		//
		//return true;
	}
#endif

}















































//---------------------------------------------------------------------
//
// text algorithms
// (they aren't templated)
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
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

	auto is_seam(char, char) -> bool;
	auto is_break(src_buf_t buf, size_t byte_idx) -> bool;
	auto prev_break(src_buf_t buf, size_t byte_idx) -> size_t;
	auto next_break(src_buf_t buf, size_t byte_idx) -> size_t;

	auto find_split_point(src_buf_t buf, size_t byte_idx, split_bias bias = split_bias::right) -> size_t;
	auto find_internal_split_point(src_buf_t buf, size_t byte_idx) -> size_t;
}

//---------------------------------------------------------------------
// 
//  visit
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename R, typename RT, typename... Fd>
	auto visit_(R default_value, node_ptr<RT> const& x, Fd&&... fs) -> R
	{
		if (x)
		{
			return x->visit(std::forward<Fd>(fs)...);
		}
		else
		{
			return default_value;
		}
	}

	template <typename RT, typename... Fd>
	auto visit_(node_ptr<RT> const& x, Fd&&... fs)
	requires std::is_same_v<void, decltype(x->visit(std::forward<Fd>(fs)...))>
	{
		if (x)
		{
			x->visit(std::forward<Fd>(fs)...);
		}
	}
}

//---------------------------------------------------------------------
// 
//  observers
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	// length :: returns sum number of characters of node & children
	template <typename RT>
	auto length(node_ptr<RT> const&) -> size_t;

	// text_info_of :: retrieves or creates a text-info from a node_ptr
	template <typename RT>
	auto text_info_of(node_ptr<RT> const&) -> text_info_t;

	// valid_children_count :: initialized children (or just counts how many children are initialized)
	//   for an internal-node. returns 0 for a leaf-node.
	template <typename RT>
	auto valid_children_count(node_ptr<RT> const& node) -> uint32_t;

	template <typename RT>
	auto is_saturated(node_info_t<RT> const&) -> bool;
}



//---------------------------------------------------------------------
//
//  tree algorithms
//  -----------------
// 
//  these functions operate on ... something
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	// find_for_char_idx :: returns <index-of-child-node, remaining-characters>
	template <typename RT>
	auto find_for_char_idx(node_internal_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>;

	// for_all_text :: visits each leaf in sequence and invokes f(std::string_view)
	template <typename F, typename RT>
	auto for_all_text(F f, node_info_t<RT> const& ri) -> void;
}


//---------------------------------------------------------------------
// 
// 
// 
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	// these may have no purpose...

	template <typename RT>
	auto replace_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& replacee)
		-> node_info_t<RT>;

	template <typename RT>
	auto append_(node_info_t<RT> const& dest, node_info_t<RT> const& insertee)
		-> node_info_t<RT>;

#if 0
	template <typename RT>
	auto insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& insertee)
		-> insert_result_t<RT>;
#endif

	template <typename RT>
	auto replace_and_insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& repl_info, maybe_node_info_t<RT> const& maybe_ins_info)
		-> insert_result_t<RT>;
}


//---------------------------------------------------------------------
//
//  mutating tree functions
//  -------------------------
// 
//  these functions "modify" the tree, which for our immutable data-
//  structure means we're eventually returning a new root node. 
//  conceptually, these algorithms have assumed that any insertion of
//  nodes will not lead to seams, i.e. you've already checked that.
// 
// 
//  NOTE: currently these are only used for naive tree-creation... so they may go away
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	//
	// node-splitting
	// ----------------
	//   given an idx these functions will return the lhs & rhs to either side
	//   of that idx. if only one child is to the left/right of the idx, that child
	//   will be returned directly at one less height. otherwise, the children
	//   as a group will be returned as children of a tree of the same height
	//
	template <typename RT>
	auto node_split_across_lhs_(tree_branch_t<RT> const&, size_t idx)
		-> tree_t<RT>;

	template <typename RT>
	auto node_split_across_rhs_(tree_branch_t<RT> const&, size_t idx)
		-> tree_t<RT>;

	template <typename RT>
	auto node_split_across_(tree_branch_t<RT> const& tree, size_t idx)
		-> std::tuple<tree_t<RT>, tree_t<RT>>;


	//
	// tree_concat_
	// -------------
	//   concatenates two trees together. they may be different heightstakes two trees of potentially differing heights, and concatenates them together.
	//
	template <typename RT>
	auto tree_concat_(node_info_t<RT> const& tree, size_t tree_height, node_info_t<RT> const& prepend_tree, size_t prepend_depth)
		-> node_info_t<RT>;


	// stitching

	template <typename RT>
	auto stitch_upwards_simple_(tree_branch_t<RT> const&, size_t child_idx, maybe_node_info_t<RT> const&)
		-> maybe_node_info_t<RT>;

	template <typename RT>
	auto stitch_upwards_(tree_branch_t<RT> const&, size_t child_idx, edit_result_t<RT> const&)
		-> edit_result_t<RT>;


	//
	// navigate_to_leaf / navigate_to_front_leaf / navigate_to_back_leaf
	//
	// this function ...
	//
	template <typename RT, typename Data, typename Fp>
	using navigate_to_leaf_result_t = std::invoke_result_t<Fp, tree_leaf_t<RT> const&, Data>;

	template <typename RT, typename Data, typename Fp>
	auto navigate_upwards_passthrough_(tree_branch_t<RT> const&, size_t, navigate_to_leaf_result_t<RT, Data, Fp> const&)
		-> navigate_to_leaf_result_t<RT, Data, Fp>;

	template <typename RT, typename Data, typename Fd, typename Fp, typename Fu = decltype(&navigate_upwards_passthrough_<RT, Data, Fp>)>
	auto navigate_to_leaf(tree_t<RT> const& info, Data, Fd&& down_fn, Fp&& payload_fn, Fu&& up_fn = navigate_upwards_passthrough_<RT, Data, Fp>)
		-> navigate_to_leaf_result_t<RT, Data, Fp>;

	// navigate_to_front_leaf

	template <typename RT, typename Fp, typename Fu>
	auto navigate_to_front_leaf(tree_t<RT> const&, Fp&& payload_fn, Fu&& up_fn);

	template <typename RT, typename Fp>
	auto navigate_to_front_leaf(tree_t<RT> const&, Fp&& payload_fn);

	// navigate_to_back_leaf

	template <typename RT, typename F, typename G = decltype(navigate_upwards_passthrough_<RT, size_t, F>)>
	auto navigate_to_back_leaf(tree_t<RT> const&, F&& downfn, G&& upfn = navigate_upwards_passthrough_<RT, size_t, F>);
}

//---------------------------------------------------------------------
//
//  text-tree functions
// 
//  these functions modify the tree by modifying the actual text
//  representation. thus, they may introduce seams
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT, typename F>
	auto edit_chunk_at_char(tree_t<RT> const& tree, size_t char_idx, F&& payload_function)
		-> edit_result_t<RT>;
}

export namespace atma::_rope_
{
	

	// mending

	template <typename RT>
	using mend_result_type = std::tuple<seam_t, std::optional<std::tuple<node_info_t<RT>, node_info_t<RT>>>>;

	template <typename RT>
	auto mend_left_seam_(seam_t, maybe_node_info_t<RT> const& sibling_node, node_info_t<RT> const& seam_node)
		-> mend_result_type<RT>;

	template <typename RT>
	auto mend_right_seam_(seam_t, node_info_t<RT> const& seam_node, maybe_node_info_t<RT> const& sibling_node)
		-> mend_result_type<RT>;
}

export namespace atma::_rope_
{
	// insert

	template <typename RT>
	auto insert(size_t char_idx, node_info_t<RT> const& dest, src_buf_t const& insbuf)
		-> edit_result_t<RT>;

}

export namespace atma::_rope_
{
	// split

	template <typename RT>
	struct split_result_t
	{
		size_t tree_height = 0;
		tree_t<RT> left, right;
	};

	template <typename RT>
	auto split(tree_t<RT> const&, size_t char_idx) -> split_result_t<RT>;

	template <typename RT>
	auto split_up_fn_(tree_branch_t<RT> const& node, size_t split_idx, split_result_t<RT> const& result)
		-> split_result_t<RT>;

	template <typename RT>
	auto split_payload_(tree_leaf_t<RT> const&, size_t char_idx)
		-> split_result_t<RT>;

}

export namespace atma::_rope_
{
	// insert_small_text_

	template <typename RT>
	auto insert_small_text_(tree_leaf_t<RT> const&, size_t char_idx, src_buf_t const& insbuf)
		-> edit_result_t<RT>;
}

export namespace atma::_rope_
{
	// drop_lf_

	template <typename RT>
	auto drop_lf_(tree_leaf_t<RT> const&, size_t char_idx)
		-> maybe_node_info_t<RT>;

	// append_lf_

	template <typename RT>
	auto append_lf_(tree_leaf_t<RT> const&, size_t char_idx)
		-> maybe_node_info_t<RT>;
}










//
// validate_rope
//
export namespace atma::_rope_
{
	struct validate_rope_t_
	{
	private:
		using check_node_result_type = std::tuple<bool, uint>;

		template <typename RT>
		static auto check_node(node_info_t<RT> const& info, size_t min_children = RT::minimum_branches)
			-> check_node_result_type;

	public:
		template <typename RT>
		auto operator ()(node_info_t<RT> const& x) const -> bool;

		template <typename RT>
		auto internal_node(node_info_t<RT> const& x) const -> bool;
	};

	constexpr validate_rope_t_ validate_rope_;
}























































//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: text_info_t
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	inline text_info_t text_info_t::from_str(char const* str, size_t sz)
	{
		ATMA_ASSERT(str);

		text_info_t r;
		for (auto x : utf8_const_range_t{str, str + sz})
		{
			r.bytes += (uint16_t)x.size_bytes();
			++r.characters;
			if (utf8_char_is_newline(x))
				++r.line_breaks;
		}

		return r;
	}
}

//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_info_t
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT>
	node_info_t<RT>::node_info_t(node_ptr<RT> const& node)
		: text_info_t{text_info_of(node)}
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


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_internal_t
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, node_info_t<RT>* dest, node_info_t<RT> const& x) -> void
	{
		atma::memory_construct_at(xfer_dest(dest + acc_count), x);
		++acc_count;
	}

	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, node_info_t<RT>* dest, maybe_node_info_t<RT> const& x) -> void
	{
		if (x)
		{
			node_internal_construct_(acc_count, dest, *x);
		}
	}

	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, node_info_t<RT>* dest, range_of_element_type<node_info_t<RT>> auto&& xs) -> void
	{
		for (auto&& x : xs)
		{
			node_internal_construct_(acc_count, dest, x);
		}
	}

	template <typename RT>
	template <typename... Args>
	inline node_internal_t<RT>::node_internal_t(uint32_t height, Args&&... args)
		: node_t<RT>{node_type_t::branch, height}
	{
		(node_internal_construct_(size_, children_.data(), std::forward<Args>(args)), ...);
	}

	template <typename RT>
	inline auto node_internal_t<RT>::length() const -> size_t
	{
		size_t result = 0;
		for (auto const& x : children())
			result += x.characters;
		return result;
	}

	template <typename RT>
	inline auto node_internal_t<RT>::calculate_combined_info() const -> text_info_t
	{
		return atma::foldl(children(), text_info_t{}, functors::add);
	}

	template <typename RT>
	inline auto node_internal_t<RT>::child_at(size_type idx) const -> node_info_t<RT> const&
	{
		ATMA_ASSERT(idx < size_);

		return children_[idx];
	}

	template <typename RT>
	inline auto node_internal_t<RT>::child_at(size_type idx) -> node_info_t<RT>&
	{
		ATMA_ASSERT(idx < size_);

		return children_[idx];
	}

	template <typename RT>
	inline auto node_internal_t<RT>::try_child_at(uint32_t idx) -> maybe_node_info_t<RT>
	{
		if (0 <= idx && idx < size_)
			return children_[idx];
		else
			return {};
	}

	template <typename RT>
	inline auto node_internal_t<RT>::append(node_info_t<RT> const& info)
	{
		ATMA_ASSERT(size_ != RT::branching_factor);
		children_[size_] = info;
		++size_;
	}

	template <typename RT>
	inline auto node_internal_t<RT>::children(size_t limit) const
	{
		limit = (limit == all_children) ? size_ : limit;
		ATMA_ASSERT(limit <= RT::branching_factor);

		return std::span<node_info_t<RT> const>(children_.data(), limit);
	}

	template <typename RT>
	template <typename F>
	inline void node_internal_t<RT>::for_each_child(F f) const
	{
		for (auto& x : children_)
		{
			if (!x.node)
				break;

			std::invoke(f, x);
		}
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_leaf_t
//
//---------------------------------------------------------------------
export namespace atma::_rope_
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
		: node_t<RT>{node_type_t::leaf, 1u}
	{
#if ATMA_ROPE_DEBUG_BUFFER
		atma::memory_value_construct(atma::xfer_dest(buf, RT::buf_size));
#endif

		(node_leaf_construct_(buf, std::forward<Args>(args)), ...);
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_t
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT>
	inline node_t<RT>::node_t(node_type_t node_type, uint32_t height)
		: node_type_(node_type)
		, height_(height)
	{
	}

	template <typename RT>
	inline constexpr bool node_t<RT>::is_internal() const
	{
		return node_type_ == node_type_t::branch;
	}

	template <typename RT>
	inline constexpr bool node_t<RT>::is_leaf() const
	{
		return node_type_ == node_type_t::leaf;
	}

	template <typename RT>
	inline auto node_t<RT>::as_branch() const -> node_internal_t<RT> const&
	{
		return static_cast<node_internal_t<RT> const&>(*this);
	}

	template <typename RT>
	inline auto node_t<RT>::as_leaf() const -> node_leaf_t<RT> const&
	{
		return static_cast<node_leaf_t<RT> const&>(*this);
	}

	template <typename RT>
	inline auto node_t<RT>::as_branch() -> node_internal_t<RT>&
	{
		return static_cast<node_internal_t<RT>&>(*this);
	}

	template <typename RT>
	inline auto node_t<RT>::as_leaf() -> node_leaf_t<RT>&
	{
		return static_cast<node_leaf_t<RT>&>(*this);
	}

	template <typename RT>
	inline auto node_t<RT>::height() const -> uint32_t
	{
		return height_;
	}

	// visit node with functors
	template <typename RT>
	template <typename... Args>
	inline auto node_t<RT>::visit(Args&&... args)
	{
		auto visitor = visit_with{std::forward<Args>(args)...};

		if (node_type_ == node_type_t::branch)
			return std::invoke(visitor, this->as_branch());
		else if (node_type_ == node_type_t::leaf)
			return std::invoke(visitor, this->as_leaf());
		else
			throw std::bad_variant_access{};
	}

	template <typename RT>
	template <typename... Args>
	inline auto node_t<RT>::visit(Args&&... args) const
	{
		auto visitor = visit_with{std::forward<Args>(args)...};

		if (node_type_ == node_type_t::branch)
			return std::invoke(visitor, this->as_branch());
		else if (node_type_ == node_type_t::leaf)
			return std::invoke(visitor, this->as_leaf());
		else
			throw std::bad_variant_access{};
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: text algorithms
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	inline auto is_seam(char x, char y) -> bool
	{
		return x == charcodes::cr && y == charcodes::lf;
	}

	inline auto is_break(src_buf_t buf, size_t byte_idx) -> bool
	{
		ATMA_ASSERT(byte_idx <= buf.size());
		
		bool const is_buffer_boundary = (byte_idx == 0 || byte_idx == buf.size());
		bool const is_utf_codepoint_boundary = (buf[byte_idx] >> 6) != 0b10;
		bool const is_not_mid_crlf = !is_buffer_boundary || !is_seam(buf[byte_idx - 1], buf[byte_idx]);

		return is_buffer_boundary || is_utf_codepoint_boundary && is_not_mid_crlf;
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

	inline auto find_split_point(src_buf_t buf, size_t byte_idx, split_bias bias) -> size_t
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
	inline auto insert_and_redistribute(text_info_t const& host, src_buf_t const& hostbuf, src_buf_t const& insbuf, size_t byte_idx)
		-> std::tuple<node_info_t<RT>, node_info_t<RT>>
	{
		ATMA_ASSERT(!insbuf.empty());
		//ATMA_ASSERT(byte_idx < hostbuf.size());
		if (byte_idx < hostbuf.size())
		{
			ATMA_ASSERT(utf8_byte_is_leading((byte const)hostbuf[byte_idx]));
		}

		// determine split point
		size_t split_idx = 0;
		size_t insbuf_split_idx = 0;
		{
			constexpr auto splitbuf_size = size_t(8);
			constexpr auto splitbuf_halfsize = splitbuf_size / 2ull;
			static_assert(splitbuf_halfsize * 2 == splitbuf_size);

			char splitbuf[splitbuf_size] = {0};

			size_t result_size = hostbuf.size() + insbuf.size();
			size_t midpoint = result_size / 2;
			size_t ins_end_idx = byte_idx + insbuf.size();

			size_t bufcopy_start = midpoint - std::min(splitbuf_halfsize, midpoint);
			size_t bufcopy_end = std::min(result_size, midpoint + splitbuf_halfsize);

			for (size_t i = bufcopy_start; i != bufcopy_end; ++i)
			{
				splitbuf[i - bufcopy_start]
					= (i < byte_idx) ? hostbuf[i]
					: (i < ins_end_idx) ? insbuf[i - byte_idx]
					: hostbuf[i - insbuf.size()]
					;
			}

			split_idx = bufcopy_start + find_split_point(
				xfer_src(splitbuf),
				midpoint - bufcopy_start);

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


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: tree observers
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT>
	auto is_saturated(node_info_t<RT> const& info) -> bool
	{
		return info.node->is_leaf() || info.children >= RT::branching_factor;
	}

	template <typename RT>
	inline auto length(node_ptr<RT> const& x) -> size_t
	{
		return visit_(size_t(), x,
			[](node_internal_t<RT> const& x) { return x.length(); },
			[](node_leaf_t<RT> const& x) { return size_t(); });
	}

	template <typename RT>
	inline auto text_info_of(node_ptr<RT> const& x) -> text_info_t
	{
		return visit_(text_info_t{}, x,
			[](node_internal_t<RT> const& x) { return x.calculate_combined_info(); },
			[](node_leaf_t<RT> const& x) { return text_info_t::from_str(x.buf.data(), x.buf.size()); });
	}

	template <typename RT>
	inline auto valid_children_count(node_ptr<RT> const& x) -> uint32_t
	{
		return visit_(uint32_t(), x,
			[](node_internal_t<RT> const& x) { return (uint32_t)x.children().size(); },
			[](node_leaf_t<RT> const& x) -> uint32_t { return 0u; });
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: non-mutating tree algorithms
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	// returns: <index-of-child-node, remaining-characters>
	template <typename RT>
	inline auto find_for_char_idx(node_internal_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		size_t child_idx = 0;
		size_t acc_chars = 0;
		for (auto const& child : x.children())
		{
			if (char_idx < acc_chars + child.characters)
				break;

			acc_chars += child.characters - child.dropped_characters;
			++child_idx;
		}

		return std::make_tuple(child_idx, char_idx - acc_chars);
	}

	template <typename F, typename RT>
	inline auto for_all_text(F f, node_info_t<RT> const& ri) -> void
	{
		visit_(ri.node,
			[f](node_internal_t<RT> const& x)
			{
				x.for_each_child(atma::curry(&for_all_text<F, RT>, f));
			},
			[&ri, f](node_leaf_t<RT> const& leaf)
			{
				auto bufview = std::string_view{leaf.buf.data() + ri.dropped_bytes, ri.bytes};
				std::invoke(f, bufview);
			});
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: mutating tree functions
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	template <typename RT>
	inline auto replace_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& repl_info) -> node_info_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "replace_: called on non-internal node");
		ATMA_ASSERT(idx < dest.children, "replace_: index out of bounds");

		auto& dest_node = dest.node->as_branch();
		auto children = dest_node.children();

		auto result_node = make_internal_ptr<RT>(
			dest_node.height(),
			xfer_src(children, idx),
			repl_info,
			xfer_src(children, idx + 1, children.size() - idx - 1));

		auto result = node_info_t<RT>{result_node};
		return result;
	}

	template <typename RT>
	inline auto replace_(tree_t<RT> const& dest, size_t idx, tree_t<RT> const& replacee) -> node_info_t<RT>
	{
		ATMA_ASSERT(idx < dest.children().size(), "replace_: index out of bounds");

		auto const& children = dest.children();

		auto result_node = make_internal_ptr<RT>(
			dest.height(),
			xfer_src(children).take(idx),
			replacee.info(),
			xfer_src(children).skip(idx + 1));

		auto result = node_info_t<RT>{result_node};
		return result;
	}

	template <typename RT>
	inline auto append_(node_info_t<RT> const& dest, node_info_t<RT> const& insertee) -> node_info_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "append_: called on non-internal node");
		ATMA_ASSERT(dest.children < RT::branching_factor, "append_: insufficient space");

		auto& dest_node = dest.node->as_branch();
		auto children = dest_node.children();

		bool insertion_is_at_back = dest.children == children.size();
		bool space_for_additional_child = children.size() < RT::branching_factor;

		if (insertion_is_at_back && space_for_additional_child)
		{
			dest_node.append(insertee);

			auto result = node_info_t<RT>{
				dest + static_cast<text_info_t const&>(insertee),
				dest.children + 1,
				dest.node};

			return result;
		}
		else
		{
			auto result_node = make_internal_ptr<RT>(
				dest_node.height(),
				xfer_src(children, dest.children),
				insertee);

			auto result = node_info_t<RT>{
				dest + static_cast<text_info_t const&>(insertee),
				dest.children + 1,
				result_node};

			return result;
		}
	}

	template <typename RT>
	inline auto append_(tree_branch_t<RT> const& dest, tree_t<RT> const& insertee) -> tree_t<RT>
	{
		ATMA_ASSERT(dest.children().size() < RT::branching_factor, "append_: insufficient space");

		auto const& children = dest.children();

		// if we are only aware of X nodes, and the underlying node has
		// exactly X nodes, then 
		bool const underlying_node_has_identical_nodes = dest.node().children().size() == children.size();
		
		if (underlying_node_has_identical_nodes && dest.node().has_space_for_another_child())
		{
			dest.node().append(insertee);

			auto result = node_info_t<RT>{
				dest + insertee,
				(uint32_t)children.size() + 1,
				dest.info().node};

			return result;
		}
		else
		{
			auto result_node = make_internal_ptr<RT>(
				dest.height(),
				children,
				insertee);

			auto result = node_info_t<RT>{
				dest + insertee,
				(uint32_t)children.size() + 1,
				result_node};

			return result;
		}
	}

#if 0
	template <typename RT>
	inline auto insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& ins_info) -> insert_result_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "insert_: called on non-internal node");
		ATMA_ASSERT(idx <= dest.children, "insert_: index out of bounds");

		auto& dest_node = dest.node->as_branch();
		auto children = dest_node.children();

		bool insertion_is_at_back = idx == children.size();
		bool space_for_additional_child = children.size() < RT::branching_factor;

		//
		// the simplest case is where:
		//   a) the insertion index is at the back of children (we're appending)
		//   b) there's space left ^_^
		//
		// in this case we can simply append to the children of the node, as the
		// destination-info won't be updated and will still address the original
		// number of children
		//
		if (insertion_is_at_back && space_for_additional_child)
		{
			dest_node.append(ins_info);
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
				dest_node.height(),
				xfer_src(children, idx),
				ins_info,
				xfer_src_between(children, idx, children.size()));

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
					dest_node.height(),
					xfer_src(children, idx),
					ins_info,
					xfer_src(children, idx, left_size - idx - 1));

				rn = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children.last(right_size)));
			}
			else
			{
				ln = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children, left_size));

				rn = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children, left_size, idx - left_size),
					ins_info,
					xfer_src(children, idx, children.size() - idx));
			}

			node_info_t<RT> lhs{ln};
			node_info_t<RT> rhs{rn};

			return insert_result_t<RT>{lhs, rhs};
		}
	}
#endif

	template <typename RT>
	inline auto insert_(tree_branch_t<RT> const& dest, size_t idx, tree_t<RT> const& insertee) -> insert_result_t<RT>
	{
		ATMA_ASSERT(idx <= dest.children().size(), "insert_: index out of bounds");

		auto const& children = dest.children();

		// slight gotcha: the info's child-count has to align with the node's child-count
		bool const insertion_is_at_back = idx == dest.backing_children().size() && idx == children.size();
		bool const space_for_additional_child = dest.node().has_space_for_another_child();

		if (space_for_additional_child && insertion_is_at_back)
		{
			// the simplest case is where:
			//   a) the insertion index is at the back of children (we're appending)
			//   b) there's space left ^_^
			//
			// in this case we can simply append to the children of the node, as the
			// destination-info won't be updated and will still address the original
			// number of children

			dest.node().as_branch().append(insertee.info());

			node_info_t<RT> result{(dest.info() + insertee.info()), dest.info().children + 1, dest.info().node};
			return {result};
		}
		else if (space_for_additional_child && !insertion_is_at_back)
		{
			// the next-simplest case is when it's an insert in the middle. simply
			// recreate the node with the child in the middle.
			
			auto result_node = make_internal_ptr<RT>(
				dest.height(),
				xfer_src(children, idx),
				insertee.info(),
				xfer_src_between(children, idx, children.size()));

			auto result = node_info_t<RT>{result_node};

			return {result};
		}
		else
		{
			// the final case - we're out of space, so we must split the node
			constexpr auto split_idx = ceil_div(RT::branching_factor, size_t(2));

			node_ptr<RT> ln, rn;

			if (idx <= split_idx)
			{
				ln = make_internal_ptr<RT>(
					dest.height(),
					xfer_src(children).to(idx),
					insertee.info(),
					xfer_src(children).from_to(idx, split_idx));

				rn = make_internal_ptr<RT>(
					dest.height(),
					xfer_src(children).from(split_idx));
			}
			else
			{
				ln = make_internal_ptr<RT>(
					dest.height(),
					xfer_src(children).to(split_idx + 1));

				rn = make_internal_ptr<RT>(
					dest.height(),
					xfer_src(children).from_to(split_idx + 1, idx),
					insertee.info(),
					xfer_src(children).from(idx));
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

		auto& dest_node = dest.node->as_branch();
		ATMA_ASSERT(idx < dest_node.children().size(), "index for replacement out of bounds");

		// in the simple case, this is just the replace operation
		if (!maybe_ins_info.has_value())
		{
			return { replace_(dest, idx, repl_info) };
		}

		auto& ins_info = maybe_ins_info.value();

		auto const children = dest_node.children();
		bool const space_for_additional_two_children = (children.size() + 1 <= RT::branching_factor);

		// we can fit the additional node in
		if (space_for_additional_two_children)
		{
			auto result_node = make_internal_ptr<RT>(
				dest_node.height(),
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
					dest_node.height(),
					xfer_src(children, idx),
					repl_info,
					ins_info,
					xfer_src(children, idx + 1, left_size - idx - 2));

				rn = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children, left_size - 1, right_size));
			}
			// case 2: nodes straddle split
			else if (idx < left_size)
			{
				ln = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children, idx),
					repl_info);

				rn = make_internal_ptr<RT>(
					dest_node.height(),
					ins_info,
					xfer_src(children, idx + 1, children.size() - idx - 1));
			}
			// case 3: nodes to the right of split
			else
			{
				ln = make_internal_ptr<RT>(
					dest_node.height(),
					xfer_src(children, left_size));

				rn = make_internal_ptr<RT>(
					dest_node.height(),
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

	#define ATMA_ROPE_VERIFY_INFO_SYNC(info) \
		do { \
			if constexpr (ATMA_ENABLE_ASSERTS) \
			{ \
				node_info_t<RT> _tmp_{(info).node}; \
				ATMA_ASSERT(_tmp_ == (info)); \
			} \
		} \
		while (0)

	template <typename RT>
	inline auto tree_merge_nodes_(node_info_t<RT> const& left, node_info_t<RT> const& right) -> node_info_t<RT>
	{
		// validate assumptions:
		//  - both nodes are leaves, or both are internal
		//  - both nodes are at the same height
		ATMA_ASSERT(left.node->is_leaf() == right.node->is_leaf());
		ATMA_ASSERT(left.node->height() == right.node->height());
		auto const height = left.node->height();

		if constexpr (ATMA_ENABLE_ASSERTS)
		{
			node_info_t<RT> right_node_info_temp{right.node};
			ATMA_ASSERT(right_node_info_temp == right);
		}

		auto result_text_info =
			static_cast<text_info_t const&>(left) +
			static_cast<text_info_t const&>(right);

		if (left.node->is_leaf())
		{
			// we know the height is two because we've just added an internal
			// node above our two leaf nodes

			auto result_node = make_internal_ptr<RT>(2u, left, right);
			return node_info_t<RT>{result_text_info, result_node};
		}

		auto const& li = left.node->as_branch();
		auto const& ri = right.node->as_branch();

		if (left.children >= RT::minimum_branches && right.children >= RT::minimum_branches)
		{
			// both nodes are saturated enough where we can just make a node above

			auto result_node = make_internal_ptr<RT>(height + 1, left, right);
			auto result = node_info_t<RT>{result_text_info, result_node};
			return result;
		}
		else if (size_t const children_count = left.children + right.children; children_count <= RT::branching_factor)
		{
			// both nodes are so UNsaturated that we can merge the children into a new node

			auto result_node = make_internal_ptr<RT>(
				height,
				li.children(left.children),
				ri.children(right.children));

			return node_info_t<RT>{result_text_info, result_node};
		}
		else
		{
			// nodes are too unbalanced. we need to redistribute. sum the nodes and
			// split in half, make sure left has majority in uneven cases

			size_t const new_right_children_count = children_count / 2;
			size_t const new_left_children_count = children_count - new_right_children_count;

			size_t const take_from_left_for_left = std::min(new_left_children_count, li.children().size());
			size_t const take_from_right_for_left = new_left_children_count - take_from_left_for_left;
			size_t const take_from_right_for_right = std::min(new_right_children_count, ri.children().size());
			size_t const take_from_left_for_right = new_right_children_count - take_from_right_for_right;

			auto new_left_node = make_internal_ptr<RT>(
				height,
				xfer_src(li.children(), take_from_left_for_left),
				xfer_src(ri.children(), take_from_right_for_left));

			node_info_t<RT> new_left_info{new_left_node};

			auto new_right_node = make_internal_ptr<RT>(
				height,
				xfer_src(li.children()).last(take_from_left_for_right),
				xfer_src(ri.children()).last(take_from_right_for_right));

			node_info_t<RT> new_right_info{new_right_node};

			auto result_node = make_internal_ptr<RT>(
				height + 1,
				new_left_info,
				new_right_info);


			if constexpr (ATMA_ENABLE_ASSERTS)
			{
				auto new_result_info = static_cast<text_info_t const&>(new_left_info) + static_cast<text_info_t const&>(new_right_info);
				ATMA_ASSERT(result_text_info == new_result_info);
			}

			return node_info_t<RT>{result_text_info, result_node};
		}
	}

	template <typename RT>
	auto node_split_across_lhs_(tree_branch_t<RT> const& branch, size_t idx) -> tree_t<RT>
	{
		if (idx == 0)
		{
			// no children I guess!
			return {make_leaf_ptr<RT>()};
		}
		else if (idx == branch.info().children - 1)
		{
			// all children from first node
			return branch;
		}
		else if (idx == 1)
		{
			return {branch.children()[0]};
		}
		else
		{
			auto tree = tree_t<RT>{make_internal_ptr<RT>(
				branch.height(),
				branch.children().subspan(0, idx))};

			return tree;
		}
	}

	template <typename RT>
	auto node_split_across_rhs_(tree_branch_t<RT> const& branch, size_t idx) -> tree_t<RT>
	{
		ATMA_ASSERT(idx < branch.info().children);

		if (idx == branch.info().children - 1)
		{
			// no children I guess!
			return {make_leaf_ptr<RT>()};
		}
		else if (idx == branch.info().children - 2)
		{
			return {branch.child_at(branch.info().children - 1)};
		}
		else
		{
			node_info_t<RT> info = make_internal_ptr<RT>(branch.height(), xfer_src(branch.children()).from(idx + 1));
			return {info};
		}
	}

	template <typename RT>
	auto node_split_across_(tree_branch_t<RT> const& tree, size_t idx) -> std::tuple<tree_t<RT>, tree_t<RT>>
	{
		return {node_split_across_lhs_(tree, idx), node_split_across_rhs_(tree, idx)};
	}

	template <typename RT>
	inline auto tree_concat_(tree_t<RT> const& left, tree_t<RT> const& right) -> tree_t<RT>
	{
		// validate assumptions:
		// - left & right are both valid trees
		ATMA_ASSERT(validate_rope_(left.info()));
		ATMA_ASSERT(validate_rope_(right.info()));

		if (left.height() < right.height())
		{
			auto const& right_branch = right.as_branch();
			auto right_children = right.children(); //info().node->as_branch().children();

			bool const left_is_saturated = is_saturated(left.info());
			bool const left_is_one_level_below_right = left.height() + 1 == right.height();

			if (left_is_saturated && left_is_one_level_below_right && right_branch.node().has_space_for_another_child())
			{
				auto n = insert_(right_branch, 0, left);
				ATMA_ASSERT(!n.maybe_rhs);
				return {n.lhs};
			}
			else if (left_is_saturated && left_is_one_level_below_right)
			{
				auto [ins_left, ins_right] = insert_(right_branch, 0, left);
				ATMA_ASSERT(ins_right);
				return tree_merge_nodes_<RT>(ins_left, *ins_right);
			}
			else
			{
				auto subtree = tree_concat_(
					left,
					{right_children.front()});

				if (subtree.height() == right.height() - 1)
				{
					auto result = replace_(right, 0, subtree);
					return result;
				}
				else if (subtree.height() == right.height())
				{
					node_info_t<RT> n2 = make_internal_ptr<RT>(
						right.height(),
						xfer_src(right_children).skip(1));

					return tree_merge_nodes_(subtree.info(), n2);
				}
				else
				{
					ATMA_HALT("oh no");
					return right;
				}
			}
		}
		else if (right.height() < left.height())
		{
			auto const& left_branch = left.as_branch();
			auto left_children = left.children(); //left.info.node->as_branch().children();

			bool const right_is_saturated = is_saturated(right.info());
			bool const right_is_one_level_below_left = right.height() + 1 == left.height();
			
			if (right_is_saturated && right_is_one_level_below_left && left_branch.node().has_space_for_another_child())
			{
				auto n = append_(left.as_branch(), right);
				//ATMA_ROPE_VERIFY_INFO_SYNC(n);
				return n;
			}
			else if (right_is_saturated && right_is_one_level_below_left)
			{
				auto [ins_left, ins_right] = insert_(left_branch, left.info().children, right);
				ATMA_ASSERT(ins_right);
				return tree_merge_nodes_<RT>(ins_left, *ins_right);
			}
			else 
			{
				auto subtree = tree_concat_(
					{left_children.back()},
					right);

				if (subtree.height() == left.height() - 1)
				{
					auto left_prime = replace_(
						left,
						left.info().children - 1,
						subtree);

					return left_prime;
				}
				else if (subtree.height() == left.height())
				{
					auto left_prime_node = make_internal_ptr<RT>(
						left.height(),
						xfer_src(left_children).drop(1));

					return tree_merge_nodes_(node_info_t<RT>{left_prime_node}, subtree.info());
				}
				else
				{
					ATMA_HALT("oh no");
					return left;
				}
			}
		}
		else
		{
			auto result = tree_merge_nodes_(left.info(), right.info());
			//ATMA_ROPE_VERIFY_INFO_SYNC(result.info);
			return result;
		}
	}














	template <typename RT, typename Data, typename Fp>
	inline auto navigate_upwards_passthrough_(tree_branch_t<RT> const&, size_t, navigate_to_leaf_result_t<RT, Data, Fp> const& x) -> navigate_to_leaf_result_t<RT, Data, Fp>
	{
		return x;
	}

	// navigate_to_leaf_selection_selector_

	template <typename RT, typename Fd, typename Data>
	inline auto navigate_to_leaf_selection_selector_(tree_branch_t<RT> const& branch, Fd&& down_fn, Data data)
	requires std::is_invocable_r_v<std::tuple<size_t>, Fd, tree_branch_t<RT> const&, Data>
	{
		// version where down_fn doesn't return data

		auto [child_idx] = std::invoke(std::forward<Fd>(down_fn), branch, data);
		auto&& child_info = branch.child_at((int)child_idx);
		return std::make_tuple(child_idx, std::ref(child_info), data);
	}

	template <typename RT, typename Fd, typename Data>
	inline auto navigate_to_leaf_selection_selector_(tree_branch_t<RT> const& branch, Fd&& down_fn, Data data)
	requires std::is_invocable_r_v<std::tuple<size_t, Data>, Fd, tree_branch_t<RT> const&, Data>
	{
		// version where down_fn (possibly) mutates data and returns it

		auto [child_idx, data_prime] = std::invoke(std::forward<Fd>(down_fn), branch, data);
		auto&& child_info = branch.child_at((int)child_idx);
		return std::make_tuple(child_idx, std::ref(child_info), data_prime);
	}



	template <typename RT, typename Data, typename Fd, typename Fp, typename Fu>
	inline auto navigate_to_leaf(tree_t<RT> const& tree, Data data, Fd&& down_fn, Fp&& payload_fn, Fu&& up_fn) -> navigate_to_leaf_result_t<RT, Data, Fp>
	{
		// the algorithm here is dead simple:
		//
		//  1) leaf nodes: call payload_fn and return that
		//  2) internal nodes:
		//     a) call down_fn to figure out which child to navigate to
		//     b) recurse onto that child
		//     c) with the result from our recursion, call up_fn and return that as the result
		//     d) fin.
		//
		return tree.info().node->visit(
			[&](node_internal_t<RT>& x)
			{
				auto [child_idx, child_info, data_prime] = navigate_to_leaf_selection_selector_(
					tree.as_branch(),
					std::forward<Fd>(down_fn),
					data);

				auto result = navigate_to_leaf(
					tree_t<RT>{child_info}, data_prime,
					std::forward<Fd>(down_fn),
					std::forward<Fp>(payload_fn),
					std::forward<Fu>(up_fn));

				return std::invoke(std::forward<Fu>(up_fn), tree.as_branch(), child_idx, result);
			},
			[&](node_leaf_t<RT>& x)
			{
				return std::invoke(std::forward<Fp>(payload_fn), tree.as_leaf(), data);
			});
	}

	template <typename RT, typename Fp, typename Fu>
	inline auto navigate_to_front_leaf(tree_t<RT> const& tree, Fp&& payload_fn, Fu&& up_fn)
	{
		return navigate_to_leaf(tree, size_t(),
			[](tree_branch_t<RT> const&, size_t) { return std::make_tuple(0); },
			std::forward<Fp>(payload_fn),
			std::forward<Fu>(up_fn));
	}

	template <typename RT, typename Fp>
	inline auto navigate_to_front_leaf(tree_t<RT> const& tree, Fp&& payload_fn)
	{
		return navigate_to_front_leaf(tree,
			std::forward<Fp>(payload_fn),
			&navigate_upwards_passthrough_<RT, size_t, Fp>);
	}

	template <typename RT, typename Fp, typename Fu>
	auto navigate_to_back_leaf(tree_t<RT> const& tree, Fp&& payload_fn, Fu&& up_fn)
	{
		return navigate_to_leaf(tree, size_t(),
			[](tree_branch_t<RT> const& branch, size_t) { return std::make_tuple(branch.info().children - 1); },
			std::forward<Fp>(payload_fn),
			std::forward<Fu>(up_fn));
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: text-tree functions
//
//---------------------------------------------------------------------
export namespace atma::_rope_
{
	// edit_chunk_at_char

	template <typename RT, typename F>
	inline auto edit_chunk_at_char(tree_t<RT> const& tree, size_t char_idx, F&& payload_function) -> edit_result_t<RT>
	{
		return navigate_to_leaf(tree, char_idx,
			tree_find_for_char_idx<RT>,
			std::forward<F>(payload_function),
			stitch_upwards_<RT>);
	}
}


export namespace atma::_rope_
{
	template <typename RT>
	inline auto stitch_upwards_simple_(tree_branch_t<RT> const& branch, size_t child_idx, maybe_node_info_t<RT> const& child_info) -> maybe_node_info_t<RT>
	{
		return (child_info)
			? maybe_node_info_t<RT>{ replace_(branch.info(), child_idx, *child_info) }
			: maybe_node_info_t<RT>{};
	}

	template <typename RT>
	inline auto stitch_upwards_(tree_branch_t<RT> const& branch, size_t child_idx, edit_result_t<RT> const& er) -> edit_result_t<RT>
	{
		auto const& [left_info, maybe_right_info, seam] = er;

		auto& node = branch.node();

		// validate that there's no seam between our two edit-result nodes. this
		// should have been caught in whatever edit operation was performed. stitching
		// the tree and melding crlf pairs is for siblings
		if (maybe_right_info && left_info.node->is_leaf())
		{
			auto const& leftleaf = left_info.node->as_leaf();
			auto const& rightleaf = maybe_right_info->node->as_leaf();

			ATMA_ASSERT(!is_seam(leftleaf.buf.back(), rightleaf.buf[maybe_right_info->dropped_bytes]));
		}

		// no seam, no worries
		if (seam == seam_t::none)
		{
			return er;
		}

		seam_t const left_seam = (seam & seam_t::left);
		seam_t const right_seam = (seam & seam_t::right);

		// the two (might be one!) nodes of our edit-result are sitting in the middle
		// two positions of these children. the potentially-stitched siblings will lie
		// on either side.
		std::array<maybe_node_info_t<RT>, 4> mended_children{
			node.try_child_at((int)child_idx - 1),
			maybe_node_info_t<RT>{left_info},
			maybe_right_info,
			node.try_child_at((int)child_idx + 1)};

		// left seam
		auto const [mended_left_seam, maybe_mended_left_nodes] = mend_left_seam_(left_seam, mended_children[0], *mended_children[1]);
		if (maybe_mended_left_nodes)
		{
			mended_children[0] = std::get<0>(*maybe_mended_left_nodes);
			mended_children[1] = std::get<1>(*maybe_mended_left_nodes);
		}

		// right seam
		auto& right_seam_node = mended_children[2] ? *mended_children[2] : *mended_children[1];
		auto const [mended_right_seam, maybe_mended_right_nodes] = mend_right_seam_(right_seam, right_seam_node, mended_children[3]);
		if (maybe_mended_right_nodes)
		{
			right_seam_node = std::get<0>(*maybe_mended_right_nodes);
			mended_children[3] = std::get<1>(*maybe_mended_right_nodes);
		}

		// validate that when we take the trailing subspan we will not
		// begin the subspan past the end. _at_ the end is fine (it's a nop)
		ATMA_ASSERT(child_idx + 2 <= node.children().size());

		auto result_node = (child_idx > 0)
			? make_internal_ptr<RT>(
				node.height() + 1,
				xfer_src(node.children().first(child_idx - 1)),
				mended_children[0], mended_children[1], mended_children[2], mended_children[3],
				xfer_src(node.children().subspan(child_idx + 1)))
			: make_internal_ptr<RT>(
				node.height() + 1,
				mended_children[0], mended_children[1], mended_children[2], mended_children[3],
				xfer_src(node.children().subspan(child_idx + 2)));

		return {result_node, maybe_node_info_t<RT>{}, mended_left_seam | mended_right_seam};
	}

	template <typename RT>
	inline auto mend_left_seam_(seam_t seam, maybe_node_info_t<RT> const& sibling_node, node_info_t<RT> const& seam_node) -> mend_result_type<RT>
	{
		if (seam == seam_t::none)
		{
			return {seam_t::none, {}};
		}
		else if (sibling_node)
		{
			// append lf to the back of the previous leaf. if this returns an empty optional,
			// then that leaf didn't have a trailing cr at the end, and we are just going to
			// leave this lf as it is... sitting all pretty at the front

			auto sibling_node_prime = navigate_to_back_leaf(tree_t<RT>{*sibling_node},
				append_lf_<RT>,
				stitch_upwards_simple_<RT>);

			if (sibling_node_prime)
			{
				// drop lf from seam node
				auto seam_node_prime = navigate_to_back_leaf(tree_t<RT>{seam_node},
					drop_lf_<RT>,
					stitch_upwards_simple_<RT>);

				ATMA_ASSERT(seam_node_prime);

				return {seam_t::none, std::make_tuple(*sibling_node_prime, *seam_node_prime)};
			}
			else
			{
				return {seam_t::none, {}};
			}
		}

		return {seam, {}};
	}

	template <typename RT>
	inline auto mend_right_seam_(seam_t seam, node_info_t<RT> const& seam_node, maybe_node_info_t<RT> const& sibling_node) -> mend_result_type<RT>
	{
		if (seam == seam_t::none)
		{
			return {seam_t::none, {}};
		}
		else if (sibling_node)
		{
			// drop lf from the front of the next leaf. if this returns an empty optional,
			// then that leaf didn't have an lf at the front, and we are just going to
			// leave this cr as is... just, dangling there

			auto sibling_node_prime = navigate_to_front_leaf(tree_t<RT>{*sibling_node},
				drop_lf_<RT>,
				stitch_upwards_simple_<RT>);

			if (sibling_node_prime)
			{
				// append lf to seamed leaf
				auto seam_node_prime = navigate_to_back_leaf(tree_t<RT>{seam_node},
					append_lf_<RT>,
					stitch_upwards_simple_<RT>);

				return {seam_t::none, std::make_tuple(*seam_node_prime, *sibling_node_prime)};
			}
			else
			{
				return {seam_t::none, {}};
			}
		}

		return {seam, {}};
	}
}

export namespace atma::_rope_
{
	template <typename RT>
	inline auto insert(size_t char_idx, node_info_t<RT> const& dest, src_buf_t const& insbuf) -> edit_result_t<RT>
	{
		if constexpr (false && "microsoft have fixed their compiler")
		{
			return edit_chunk_at_char(dest, char_idx, std::bind(insert_small_text_<RT>, arg1, arg2, arg3, insbuf));
		}
		else
		{
			return edit_chunk_at_char(tree_t<RT>{dest}, char_idx,
				[&](tree_leaf_t<RT> const& leaf, size_t char_idx) { return insert_small_text_<RT>(leaf, char_idx, insbuf); });
		}
	}

	template <typename RT>
	inline auto split_payload_(tree_leaf_t<RT> const& leaf, size_t char_idx) -> split_result_t<RT>
	{
		auto split_idx = leaf.byte_idx_from_char_idx(char_idx);

		if (split_idx == 0)
		{
			// we are asking to split directly at the front of our buffer.
			// so... we're not doing that. we're instead return a no-result
			// for the lhs, and for the rhs we can just pass along our
			// existing node, as it does not need to be modified

			return {1, tree_leaf_t<RT>{make_leaf_ptr<RT>()}, leaf};
		}
		else if (split_idx == leaf.info().bytes)
		{
			// in this case, we are asking to split at the end of our buffer.
			// let's instead return a no-result for the rhs, and pass this
			// node on the lhs.

			return {1, leaf, tree_leaf_t<RT>{make_leaf_ptr<RT>()}};
		}
		else
		{
			// okay, our char-idx has landed us in the middle of our buffer.
			// split the buffer into two and return both nodes.

			auto left = tree_leaf_t<RT>{make_leaf_ptr<RT>(leaf.data())};
			auto right = tree_leaf_t<RT>{make_leaf_ptr<RT>(leaf.data())};

			return {1, left, right};
		}
	}

	template <typename RT>
	inline auto split_up_fn_(tree_branch_t<RT> const& dest, size_t split_idx, split_result_t<RT> const& result) -> split_result_t<RT>
	{
		auto const& [orig_height, split_left, split_right] = result;
		
		// "we", in this stack-frame, are one level higher than what was returned in result
		size_t const our_height = orig_height + 1;

		// validate assumptions:
		//
		//  - split_idx is valid
		//
		ATMA_ASSERT(split_idx < dest.children().size());
		//
		// validate invariants of the split-nodes' children, if the split-nodes
		// were internal nodes (and not leaf nodes, obv).
		//
		// we do this because it's possible our children are no longer valid after
		// splitting - that's okay, as long as THEIR children are valid, we can
		// in this step rearrange those sub-children into new direct children of us
		// to balance everything
		//
		// this also means for some of our shortcut algorithms where we return a sub-child
		// node we are not breaking our recursive algorithm
		//
		if constexpr (false)
		{
			if (split_left.info && (*split_left.info).node->is_internal())
			{
				auto const& lin = split_left.info->node->as_branch();
				for (auto const& x : lin.children())
				{
					ATMA_ASSERT(validate_rope_.internal_node(x));
				}
			}

			if (split_right.info && (*split_right.info).node->is_internal())
			{
				auto const& lin = split_right.info->node->as_branch();
				for (auto const& x : lin.children())
				{
					ATMA_ASSERT(validate_rope_.internal_node(x));
				}
			}
		}

		if (split_idx == 0)
		{
			// with split_idx equalling zero, the node at idx 0 has been split in two, and
			// thus there's no LHS upon which to merge split_left - we simply pass along
			// split_left up the callstack. the RHS must be joined to split_right appropriately

			if (split_right.is_saturated() && split_right.height() == orig_height)
			{
				// optimization: if split_right is a valid child-node, just change idx 0 to split_right

				auto right = replace_(tree_t<RT>{dest}, split_idx, split_right);
				return {our_height, split_left, right};
			}
			else
			{
				auto rhs_children = node_split_across_rhs_<RT>(
					dest,
					split_idx);

				auto right = tree_concat_<RT>(
					split_right,
					rhs_children);

				return {our_height, split_left, right};
			}
		}
		else if (split_idx == dest.info().children - 1)
		{
			if (split_left.is_saturated() && split_left.height() == orig_height)
			{
				// optimization: if split_left is a valid child-node, just replace
				// the last node of our children to split_left

				auto left = replace_(tree_t<RT>{dest}, split_idx, split_left);
				return {our_height, left, split_right};
			}
			else
			{
				auto lhs_children = node_split_across_lhs_<RT>(
					dest,
					split_idx);

				auto left = tree_concat_<RT>(
					lhs_children,
					split_left);

				return {our_height, left, split_right};
			}
		}
		else
		{
			auto [our_lhs_children, our_rhs_children] = node_split_across_<RT>(
				dest,
				split_idx);

			auto left = tree_concat_<RT>(
				our_lhs_children,
				split_left);

			auto right = tree_concat_<RT>(
				split_right,
				our_rhs_children);

			return {our_height, left, right};
		}
	}

	// returns: <index-of-child-node, remaining-characters>
	template <typename RT>
	inline auto tree_find_for_char_idx(tree_branch_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		return find_for_char_idx(x.info().node->as_branch(), char_idx);
	}

	template <typename RT>
	inline auto split(tree_t<RT> const& tree, size_t char_idx) -> split_result_t<RT>
	{
		return navigate_to_leaf(tree, char_idx,
			&tree_find_for_char_idx<RT>,
			&split_payload_<RT>,
			&split_up_fn_<RT>);
	}
}

export namespace atma::_rope_
{
	// insert_small_text_
	template <typename RT>
	inline auto insert_small_text_2_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t char_idx, src_buf_t insbuf) -> edit_result_t<RT>;

	template <typename RT>
	inline auto insert_small_text_(tree_leaf_t<RT> const& leaf, size_t char_idx, src_buf_t const& insbuf) -> edit_result_t<RT>
	{
		return insert_small_text_2_(leaf.info(), leaf.node(), char_idx, insbuf);
	}

	template <typename RT>
	inline auto insert_small_text_2_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t char_idx, src_buf_t insbuf) -> edit_result_t<RT>
	{
		ATMA_ASSERT(insbuf.size() <= RT::buf_edit_max_size);

		auto& buf = leaf.buf;

		// early out
		if (insbuf.empty())
		{
			return {leaf_info};
		}

		// check left seam
		//
		// determine if the first character in our incoming text is an lf character,
		// and we're trying to insert this text at the front of this chunk. if we
		// are doing that, then we want to insert the lf character in the previous
		// logical chunk, so if it may tack onto any cr character at the end of that
		// previous chunk, resulting in us only counting them as one line-break
		bool const inserting_at_front = char_idx == 0;
		bool const lf_at_front = insbuf[0] == charcodes::lf;
		seam_t const left_seam = (inserting_at_front && lf_at_front) ? seam_t::left : seam_t::none;
		if (left_seam != seam_t::none)
		{
			insbuf = insbuf.skip(1);
		}

		// check right seam
		//
		// words
		bool const inserting_at_end = char_idx == leaf_info.characters;
		bool const cr_at_end = insbuf[insbuf.size() - 1] == charcodes::cr;
		seam_t const right_seam = (inserting_at_end && cr_at_end) ? seam_t::right : seam_t::none;


		// calculate byte index from character index
		auto byte_idx = utf8_charseq_idx_to_byte_idx(buf.data() + leaf_info.dropped_bytes, leaf_info.bytes, char_idx);


		// if we want to append, we must first make sure that the (immutable, remember) buffer
		// does not have any trailing information used by other trees. secondly, we must make
		// sure that the byte_idx of the character is actually the last byte_idx of the buffer
		bool buf_is_appendable = (leaf_info.dropped_bytes + leaf_info.bytes) == buf.size();
		bool byte_idx_is_at_end = leaf_info.bytes == byte_idx;
		bool can_fit_in_chunk = leaf_info.bytes + insbuf.size() <= RT::buf_edit_max_size;

		ATMA_ASSERT(inserting_at_end == byte_idx_is_at_end);

		// simple append is possible
		if (can_fit_in_chunk && inserting_at_end && buf_is_appendable)
		{
			buf.append(insbuf);

			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;

			return _rope_::edit_result_t<RT>{result_info, {}, left_seam | right_seam};
		}
		// we're not appending, but we can fit within the chunk: reallocate & insert
		else if (can_fit_in_chunk)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;

			auto orig_buf_src = xfer_src(buf, leaf_info.dropped_bytes, leaf_info.bytes);

			if (inserting_at_end)
			{
				result_info.node = _rope_::make_leaf_ptr<RT>(
					orig_buf_src.take(byte_idx),
					insbuf);
			}
			else
			{
				result_info.node = _rope_::make_leaf_ptr<RT>(
					orig_buf_src.take(byte_idx),
					insbuf,
					orig_buf_src.skip(byte_idx));
			}

			return _rope_::edit_result_t<RT>(result_info, {}, left_seam | right_seam);
		}
		// any other case: splitting is required
		else
		{
			auto [lhs_info, rhs_info] = _rope_::insert_and_redistribute<RT>(leaf_info, xfer_src(buf), insbuf, byte_idx);

			return _rope_::edit_result_t<RT>{lhs_info, rhs_info, left_seam | right_seam};
		}
	}
}

export namespace atma::_rope_
{
#define ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(info, buf) \
	do { \
		ATMA_ASSERT((info).dropped_bytes + (info).bytes == (buf).size()); \
	} while(0)


	template <typename RT>
	inline auto drop_lf_(tree_leaf_t<RT> const& leaf, size_t) -> maybe_node_info_t<RT>
	{
		//ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(leaf_info, leaf.buf);

		if (leaf.data()[0] == charcodes::lf)
		{
			ATMA_ASSERT(leaf.info().line_breaks > 0);

			auto result = leaf.info()
				+ text_info_t{.dropped_bytes = 1, .dropped_characters = 1}
				- text_info_t{.bytes = 1, .characters = 1, .line_breaks = 1};

			return { result };
		}
		else
		{
			return {};
		}
	}

	template <typename RT>
	inline auto append_lf_(tree_leaf_t<RT> const& leaf, size_t) -> maybe_node_info_t<RT>
	{
		//ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(leaf_info, leaf.buf);
		
		// validate assumptions:
		// 
		//  - we have a cr character at the end of our buffer
		// 
		//  - the byte-index of this cr character is within buf_edit_max_size
		// 
		//  - there is space at the end of buffer to append an lf. this size is
		//    allowed to exceed buf_edit_max_size, and instead go all the way up
		//    to the limit of the buffer. this assumption is validated by the
		//    previous two points
		//
		bool const has_trailing_cr = !leaf.data().empty() && leaf.data().back() == charcodes::cr;
		if (!has_trailing_cr)
		{
			return {};
		}

		bool const can_fit_in_chunk = leaf.node().buf.size() + 1 <= RT::buf_size;
		ATMA_ASSERT(can_fit_in_chunk);

		leaf.node().buf.push_back(charcodes::lf);
		auto result_info = leaf.info() + text_info_t{.bytes = 1, .characters = 1};
		return result_info;
	}
}














































































export namespace atma::_rope_
{
	template <typename RT>
	inline auto validate_rope_t_::check_node(node_info_t<RT> const& info, size_t min_children) -> check_node_result_type
	{
		ATMA_ASSERT(info.node);

		return info.node->visit(
			[&info, min_children](node_internal_t<RT> const& internal_node)
			{
				if (info.children < min_children)
					return std::make_tuple(false, uint());

				auto r = singular_result(internal_node.children(), atma::bind_from<1>(&check_node<RT>, RT::minimum_branches));

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

	template <typename RT>
	inline auto validate_rope_t_::operator ()(node_info_t<RT> const& x) const -> bool
	{
		// assume we're passed the root (hence '2' as the minimum branch)
		return std::get<0>(check_node(x, 2));
	}

	template <typename RT>
	inline auto validate_rope_t_::internal_node(node_info_t<RT> const& x) const -> bool
	{
		return std::get<0>(check_node(x, RT::minimum_branches));
	}
}








export namespace atma::_rope_
{
	template <typename RT>
	struct build_rope_t_
	{
		inline static constexpr size_t max_level_size = RT::branching_factor + ceil_div<size_t>(RT::branching_factor, 2u);

		using level_t = std::tuple<uint32_t, std::array<node_info_t<RT>, max_level_size>>;
		using level_stack_t = std::list<level_t>;

		auto stack_level(level_stack_t& stack, uint32_t level) const -> level_t&;
		decltype(auto) stack_level_size(level_stack_t& stack, uint32_t level) const { return std::get<0>(stack_level(stack, level)); }
		decltype(auto) stack_level_array(level_stack_t& stack, uint32_t level) const { return std::get<1>(stack_level(stack, level)); }
		decltype(auto) stack_level_size(level_t const& x) const { return std::get<0>(x); }
		decltype(auto) stack_level_array(level_t const& x) const { return std::get<1>(x); }

		void stack_push(level_stack_t& stack, uint32_t level, node_info_t<RT> const& insertee) const;
		auto stack_finish(level_stack_t& stack) const -> node_info_t<RT>;

		auto operator ()(src_buf_t str) const -> basic_rope_t<RT>;
	};

	template <typename RT>
	constexpr build_rope_t_<RT> build_rope_;
}

export namespace atma::_rope_
{
	template <typename RT>
	inline auto build_rope_t_<RT>::stack_level(level_stack_t& stack, uint32_t level) const -> level_t&
	{
		auto i = stack.begin();
		std::advance(i, level);
		return *i;
	}

	template <typename RT>
	inline void build_rope_t_<RT>::stack_push(level_stack_t& stack, uint32_t level, node_info_t<RT> const& insertee) const
	{
		// if we're asking for one level more than we currently have, assume it's
		// a correctly functioning request and create it
		if (level == stack.size())
		{
			stack.emplace_back();
		}

		// if we've reached maximum capacity of this level, then we _definitely_
		// have enough nodes to create at least two nodes one level higher
		if (stack_level_size(stack, level) == max_level_size)
		{
			auto& array = stack_level_array(stack, level);

			// take the @bf leftmost nodes and construct a node one level higher
			// note: level is zero-indexed, so +2
			node_info_t<RT> higher_level_node{make_internal_ptr<RT>(
				uint32_t(level + 2),
				xfer_src(array.data(), RT::branching_factor))};

			// erase those node-infos and move the trailing ones to the front
			// don't forget to reset the size
			{
				auto const remainder_size = max_level_size - RT::branching_factor;

				atma::memory_default_construct(
					xfer_dest(array.data(), RT::branching_factor));

				atma::memory_copy(
					xfer_dest(array.data(), remainder_size),
					xfer_src(array.data() + RT::branching_factor));

				atma::memory_default_construct(
					xfer_dest(array.data() + RT::branching_factor, remainder_size));

				stack_level_size(stack, level) = remainder_size;
			}

			// insert our higher-level node into our stack one level higher
			stack_push(stack, level + 1, higher_level_node);
		}

		// insert our node
		auto& array = stack_level_array(stack, level);
		array[stack_level_size(stack, level)++] = insertee;
	}

	template <typename RT>
	inline auto build_rope_t_<RT>::stack_finish(level_stack_t& stack) const -> node_info_t<RT>
	{
		uint32_t level = 0;
		for (auto level_iter = stack.begin(); level_iter != stack.end(); ++level_iter, ++level)
		{
			auto const level_size = stack_level_size(*level_iter);
			auto const& level_array = stack_level_array(*level_iter);

			if (level == stack.size() - 1 && level_size == 1)
			{
				// we're at the top-most level and we have one node. that
				// means we have our root. this is our terminating condition.

				return level_array[0];
			}
			else if (level_size <= RT::branching_factor)
			{
				// we don't have too many nodes (by design of algorithm, we
				// have between branching-facotr/2 to branching-factor), so
				// we can put them into one internal node and insert that above

				node_info_t<RT> higher_level_node{make_internal_ptr<RT>(
					uint32_t(level + 2),
					xfer_src(level_array.data(), level_size))};

				stack_push(stack, level + 1, higher_level_node);
			}
			else
			{
				// we have too many nodes to roll up into one internal node, so
				// split the remaining nodes into two, making sure to balance
				// them as to not invalidate the invariants of the btree

				auto const lhs_size = ceil_div<size_t>(level_size, 2);
				auto const rhs_size = level_size - lhs_size;

				node_info_t<RT> higher_level_lhs_node{make_internal_ptr<RT>(
					uint32_t(level + 2),
					xfer_src(level_array.data(), lhs_size))};

				node_info_t<RT> higher_level_rhs_node{make_internal_ptr<RT>(
					uint32_t(level + 2),
					xfer_src(level_array.data() + lhs_size, rhs_size))};

				stack_push(stack, level + 1, higher_level_lhs_node);
				stack_push(stack, level + 1, higher_level_rhs_node);
			}
		}

		return node_info_t<RT>{};
	}

	template <typename RT>
	inline auto build_rope_t_<RT>::operator ()(src_buf_t str) const -> basic_rope_t<RT>
	{
		// remove null terminator if necessary
		if (str[str.size() - 1] == '\0')
			str = str.take(str.size() - 1);

		level_stack_t stack;

		// case 1. the str is small enough to just be inserted as a leaf
		while (!str.empty())
		{
			size_t candidate_split_idx = std::min(str.size(), RT::buf_size);
			auto split_idx = find_split_point(str, candidate_split_idx, split_bias::hard_left);

			auto leaf_text = str.take(split_idx);
			str = str.skip(split_idx);

			auto new_leaf = node_info_t<RT>{make_leaf_ptr<RT>(leaf_text)};

			stack_push(stack, 0, new_leaf);
		}

		// stack fixup
		auto r = stack_finish(stack);
		basic_rope_t<RT> result{r};
		return result;
	}
}

























































export namespace atma::_rope_
{
	class append_node_t_
	{
		template <typename RT>
		auto append_node_(node_info_t<RT> const& dest, node_ptr<RT> const& ins) const -> insert_result_t<RT>;

	public:
		template <typename RT>
		auto operator ()(node_info_t<RT> const& dest, node_ptr<RT> x) const -> node_info_t<RT>;
	};

	constexpr class append_node_t_ append_node_;
}

export namespace atma::_rope_
{
	template <typename RT>
	inline auto append_node_t_::append_node_(node_info_t<RT> const& dest, node_ptr<RT> const& ins) const -> insert_result_t<RT>
	{
		return dest.node->visit(
			[&](node_internal_t<RT> const& x)
			{
				auto const children = x.children();
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
					return insert_(tree_branch_t<RT>{dest}, children.size(), tree_t<RT>{node_info_t<RT>{ins}});
				}
			},
			[&](node_leaf_t<RT> const& x)
			{
				// the only way we can be here is if the root was a leaf node
				// we must root-split.
				return insert_result_t<RT>{dest, node_info_t<RT>{ins}};
			});
	}

	template <typename RT>
	auto append_node_t_::operator ()(node_info_t<RT> const& dest, node_ptr<RT> x) const -> node_info_t<RT>
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
				return node_info_t<RT>{make_internal_ptr<RT>(dest.node->height() + 1, lhs, *rhs)};
			}
			else
			{
				return lhs;
			}
		}
	};




	template <typename RT>
	inline auto build_rope_naive(src_buf_t str) -> node_info_t<RT>
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

}












































export namespace atma
{
	template <typename RT>
	inline basic_rope_t<RT>::basic_rope_t()
		: root_(_rope_::make_leaf_ptr<RT>())
	{}

	template <typename RT>
	inline basic_rope_t<RT>::basic_rope_t(_rope_::node_info_t<RT> const& root_info)
		: root_(root_info)
	{}

	template <typename RT>
	inline auto basic_rope_t<RT>::operator == (std::string_view string) const -> bool
	{
		auto iter = basic_rope_char_iterator_t<RT>{*this};

		auto blah = utf8_const_range_t{string.begin(), string.end()};
		for (auto&& x : blah)
		{
			if (*iter != x)
				return false;
			++iter;
		}

		return true;
	}

	template <typename RT>
	inline auto operator == (basic_rope_t<RT> const& lhs, basic_rope_t<RT> const& rhs) -> bool
	{
		if (static_cast<_rope_::tree_t<RT> const&>(lhs.root()).size_bytes() != static_cast<_rope_::tree_t<RT> const&>(rhs.root()).size_bytes())
			return false;

		_rope_::basic_leaf_iterator_t<RT> lhs_leaf_iter{lhs};
		_rope_::basic_leaf_iterator_t<RT> rhs_leaf_iter{rhs};
		_rope_::basic_leaf_iterator_t<RT> sentinel;

		// loop through leaves for both ropes
		while (lhs_leaf_iter != sentinel && rhs_leaf_iter != sentinel)
		{
			auto lhsd = lhs_leaf_iter->data();
			auto rhsd = rhs_leaf_iter->data();

			if (auto r = memory_compare(lhsd, rhsd); r == 0)
			{
				// memory contents compared completely, keep going
				++lhs_leaf_iter;
				++rhs_leaf_iter;
			}
			else if (r == -(int)std::size(lhsd))
			{
				
			}

		}

		return true;
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::push_back(char const* str, size_t sz) -> void
	{
		this->insert(root_.characters, str, sz);
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::insert(size_t char_idx, char const* str, size_t sz) -> void
	{
		// determine if we've got a "big" string
		//  - if we do, make a new rope and splice it into our rope
		//  - if we don't, chunkify it (even for one chunk), and insert piece-by-piece

		_rope_::edit_result_t<RT> edit_result;
		if (sz <= RT::buf_edit_max_size)
		{
			edit_result = _rope_::insert(char_idx, root_, xfer_src(str, sz));
		}
		else if (sz <= RT::buf_edit_max_size * 6)
		{
			// chunkify
			for (size_t i = 0; i != sz; )
			{
				[[maybe_unused]] size_t chunk_size = std::min((sz - i), RT::buf_edit_max_size);
				
			}
		}
		else
		{
			auto [depth, left, right] = _rope_::split(_rope_::tree_t<RT>{root_}, char_idx);
			auto ins = _rope_::build_rope_<RT>(xfer_src(str, sz));

			//std::cout << "left:>" << basic_rope_t{left.info()} << "<\n\n" << std::endl;
			//std::cout << "right:>" << basic_rope_t{right.info()} << "<\n\n" << std::endl;

			auto tr1 = _rope_::tree_concat_<RT>(
				_rope_::tree_t<RT>{left.info()},
				_rope_::tree_t<RT>{ins.root()});

			auto tr2 = _rope_::tree_concat_<RT>(
				tr1,
				right.info());

			edit_result.left = tr2.info();
		}

		if (edit_result.right.has_value())
		{
			(_rope_::text_info_t&)root_ = edit_result.left + *edit_result.right;

			root_.node = _rope_::make_internal_ptr<RT>(
				edit_result.left.node->height() + 1,
				edit_result.left,
				*edit_result.right);
		}
		else
		{
			root_ = edit_result.left;
		}
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::split(size_t char_idx) const -> std::tuple<basic_rope_t<RT>, basic_rope_t<RT>>
	{
		auto [depth, left, right] = _rope_::split<RT>(root_, char_idx);
		return { basic_rope_t{left.info()}, basic_rope_t{right.info()}};
	}


	template <typename RT>
	template <typename F>
	inline auto basic_rope_t<RT>::for_all_text(F&& f) const
	{
		_rope_::for_all_text(std::forward<F>(f), root_);
	}

	template <typename RT>
	inline std::ostream& operator << (std::ostream& stream, basic_rope_t<RT> const& x)
	{
		x.for_all_text([&stream](std::string_view str) { stream << str; });
		return stream;
	}



	using rope_t = basic_rope_t<rope_default_traits>;
}











export namespace atma
{
	template <typename RT>
	inline basic_rope_char_iterator_t<RT>::basic_rope_char_iterator_t(basic_rope_t<RT> const& rope)
		: rope_(rope)
	{
		using namespace _rope_;

		std::tie(leaf_, leaf_characters_) = navigate_to_front_leaf<RT>(rope.root(), [](tree_leaf_t<RT> const& leaf, size_t)
			{
				return std::make_tuple(node_leaf_ptr<RT>{&leaf.node()}, leaf.info().characters);
			});
	}

	template <typename RT>
	inline auto basic_rope_char_iterator_t<RT>::operator ++() -> basic_rope_char_iterator_t<RT>&
	{
		using namespace _rope_;

		++idx_;
		++rel_idx_;

		if (idx_ == rope_.size())
			return *this;

		if (rel_idx_ == leaf_characters_)
		{
			// exceeded this leaf, time to move to next leaf
			std::tie(leaf_, leaf_characters_) = navigate_to_leaf<RT>(tree_t<RT>{rope_.root()}, idx_,
				&tree_find_for_char_idx<RT>,
				[](tree_leaf_t<RT> const& leaf, size_t)
				{
					return std::make_tuple(node_leaf_ptr<RT>{&leaf.node()}, leaf.info().characters);
				});

			rel_idx_ = 0;
		}

		return *this;
	}

	template <typename RT>
	inline auto basic_rope_char_iterator_t<RT>::operator *() -> utf8_char_t
	{
		return utf8_char_t{leaf_->buf.data() + rel_idx_};
	}
}





namespace atma::_rope_
{
	template <typename RT>
	inline basic_leaf_iterator_t<RT>::basic_leaf_iterator_t(basic_rope_t<RT> const& rope)
	{
		tree_stack_.push_back({rope.root(), 0});
		while (std::get<0>(tree_stack_.back()).children()[0].node->is_internal())
		{
			tree_stack_.emplace_back(std::get<0>(tree_stack_.back()).children().front(), 0);
		}
	}

	template <typename RT>
	inline auto basic_leaf_iterator_t<RT>::operator ++() -> basic_leaf_iterator_t<RT>&
	{
		auto* parent = &std::get<0>(tree_stack_.back());
		auto* idx = &std::get<1>(tree_stack_.back());

		++*idx;

		while (*idx == parent->children().size())
		{
			tree_stack_.pop_back();

			if (tree_stack_.empty())
			{
				return *this;
			}
			else
			{
				parent = &std::get<0>(tree_stack_.back());
				idx = &std::get<1>(tree_stack_.back());
				++*idx;
			}
		}

		return *this;
	}

	template <typename RT>
	inline auto basic_leaf_iterator_t<RT>::operator *() -> tree_leaf_t<RT>
	{
		auto const& [branch, idx] = tree_stack_.back();

		return tree_leaf_t<RT>{branch.child_at(idx)};
	}
}


#endif


