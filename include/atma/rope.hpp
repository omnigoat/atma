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
#include <list>
#include <ranges>

#define ATMA_ROPE_DEBUG_BUFFER 1

import atma.bind;
import atma.types;
import atma.memory;



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
namespace atma::_rope_
{
	struct text_info_t;
	template <typename> struct node_info_t;
	template <typename RT> using maybe_node_info_t = std::optional<node_info_t<RT>>;
	template <typename RT> using maybe_node_info_ref_t = std::optional<std::reference_wrapper<node_info_t<RT> const>>;

	template <typename> struct node_internal_t;
	template <typename> struct node_leaf_t;
	template <typename> struct node_t;

	template <typename RT> using node_ptr = intrusive_ptr<node_t<RT>>;

	using dest_buf_t = dest_bounded_memxfer_t<char>;
	using src_buf_t = src_bounded_memxfer_t<char const>;

	namespace charcodes
	{
		constexpr char null = 0x0;

		constexpr char cr = 0x0d;
		constexpr char lf = 0x0a;
	}
}


//
// traits
//
namespace atma
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
namespace atma::_rope_
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
namespace atma::_rope_
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

		auto child_at(size_t idx) const -> node_info_t<RT> const& { return children_[idx]; }
		auto child_at(size_t idx) -> node_info_t<RT>& { return children_[idx]; }
		
		auto try_child_at(int idx) -> maybe_node_info_t<RT>
		{
			if (0 <= idx && idx < size_)
				return children_[idx];
			else
				return {};
		}

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

		auto children(size_t limit = all_children) const
		{
			limit = (limit == all_children) ? size_ : limit;
			ATMA_ASSERT(limit <= RT::branching_factor);

			return std::span<node_info_t<RT> const>(children_.data(), limit);
		}

		auto clone() const -> node_ptr<RT>;

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

		auto clone() const -> node_ptr<RT>;

		auto byte_idx_from_char_idx(text_info_t const&, size_t char_idx) const -> size_t;

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
		node_t(node_internal_t<RT> const&);
		node_t(node_leaf_t<RT> const&);

		bool is_internal() const;
		bool is_leaf() const;

		auto known_internal() -> node_internal_t<RT>&;
		auto known_leaf() -> node_leaf_t<RT>&;

		auto clone() -> node_ptr<RT>;

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



//---------------------------------------------------------------------
//
// text algorithms
// (they aren't templated)
// 
//---------------------------------------------------------------------
namespace atma::_rope_
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
namespace atma::_rope_
{
	template <typename R, typename RT, typename... Fd>
	auto visit_(R default_value, node_ptr<RT> const& x, Fd... fs) -> R
	{
		return !x ? default_value : std::invoke(&node_t<RT>::template visit<std::remove_reference_t<Fd>...>, *x, std::forward<Fd>(fs)...);
	}

	template <typename RT, typename... Fd>
	auto visit_(node_ptr<RT> const& x, Fd... fs)
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
namespace atma::_rope_
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
}



//---------------------------------------------------------------------
//
//  tree algorithms
//  -----------------
// 
//  these functions operate on ... something
// 
//---------------------------------------------------------------------
namespace atma::_rope_
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
namespace atma::_rope_
{
	// these may have no purpose...

	template <typename RT>
	auto replace_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& replacee)
		-> node_info_t<RT>;

	template <typename RT>
	auto append_(node_info_t<RT> const& dest, node_info_t<RT> const& insertee)
		-> node_info_t<RT>;

	template <typename RT>
	auto insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& insertee)
		-> insert_result_t<RT>;

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
namespace atma::_rope_
{
	template <typename RT>
	struct tree_and_height_t
	{
		node_info_t<RT> info;
		size_t height = 0;
	};

	template <typename RT>
	auto node_split_across_lhs_(tree_and_height_t<RT> const&, size_t idx)
		-> tree_and_height_t<RT>;

	template <typename RT>
	auto node_split_across_rhs_(tree_and_height_t<RT> const&, size_t idx)
		-> tree_and_height_t<RT>;

	template <typename RT>
	auto node_split_across_(tree_and_height_t<RT> const& tree, size_t idx)
		-> std::tuple<tree_and_height_t<RT>, tree_and_height_t<RT>>;


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
	auto stitch_upwards_simple_(node_info_t<RT> const&, node_internal_t<RT>&, size_t child_idx, maybe_node_info_t<RT> const&)
		-> maybe_node_info_t<RT>;

	template <typename RT>
	auto stitch_upwards_(node_info_t<RT> const&, node_internal_t<RT>&, size_t child_idx, edit_result_t<RT> const&)
		-> edit_result_t<RT>;

	//
	// navigate_to_leaf / navigate_to_front_leaf / navigate_to_back_leaf
	//
	// this function ...
	//
	template <typename RT, typename Data, typename Fp>
	using navigate_to_leaf_result_t = std::invoke_result_t<Fp, node_info_t<RT> const&, node_leaf_t<RT>&, Data>;

	template <typename RT, typename Data, typename Fp>
	auto navigate_upwards_passthrough_(node_info_t<RT> const&, node_internal_t<RT>&, size_t, navigate_to_leaf_result_t<RT, Data, Fp> const&)
		-> navigate_to_leaf_result_t<RT, Data, Fp>;

	template <typename RT, typename Data, typename Fd, typename Fp, typename Fu>
	auto navigate_to_leaf(node_info_t<RT> const& info, Data, Fd&& down_fn, Fp&& payload_fn, Fu&& up_fn)
		-> navigate_to_leaf_result_t<RT, Data, Fp>;

	// navigate_to_front_leaf

	template <typename RT, typename Fp, typename Fu>
	auto navigate_to_front_leaf(node_info_t<RT> const& info, Fp&& payload_fn, Fu&& up_fn);

	template <typename RT, typename Fp>
	auto navigate_to_front_leaf(node_info_t<RT> const& info, Fp&& payload_fn);

	// navigate_to_back_leaf

	template <typename RT, typename F, typename G = decltype(navigate_upwards_passthrough_<RT, size_t, F>)>
	auto navigate_to_back_leaf(node_info_t<RT> const& info, F&& downfn, G&& upfn = navigate_upwards_passthrough_<RT, size_t, F>);
}

//---------------------------------------------------------------------
//
//  text-tree functions
// 
//  these functions modify the tree by modifying the actual text
//  representation. thus, they may introduce seams
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT, typename F>
	auto edit_chunk_at_char(node_info_t<RT> const& info, size_t char_idx, F&& f)
		-> edit_result_t<RT>;
}

namespace atma::_rope_
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

namespace atma::_rope_
{
	// insert

	template <typename RT>
	auto insert(size_t char_idx, node_info_t<RT> const& dest, src_buf_t const& insbuf)
		-> edit_result_t<RT>;

}

namespace atma::_rope_
{
	// split

	template <typename RT>
	struct split_result_t
	{
		size_t tree_height = 0;
		tree_and_height_t<RT> left, right;
	};

	template <typename RT>
	auto split(node_info_t<RT> const&, size_t char_idx) -> split_result_t<RT>;
}

namespace atma::_rope_
{
	// insert_small_text_

	template <typename RT>
	auto insert_small_text_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>&, size_t char_idx, src_buf_t insbuf)
		-> edit_result_t<RT>;
}

namespace atma::_rope_
{
	// drop_lf_

	template <typename RT>
	auto drop_lf_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>&, size_t char_idx)
		-> maybe_node_info_t<RT>;

	// append_lf_

	template <typename RT>
	auto append_lf_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>&, size_t char_idx)
		-> maybe_node_info_t<RT>;
}








//---------------------------------------------------------------------
//
//  rope
//
//---------------------------------------------------------------------
namespace atma
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

		decltype(auto) root() { return (root_); }

	private:
		basic_rope_t(_rope_::node_info_t<RopeTraits> const&);

	private:
		_rope_::node_info_t<RopeTraits> root_;
	};
}



//
// validate_rope
//
namespace atma::_rope_
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
namespace atma::_rope_
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
namespace atma::_rope_
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
namespace atma::_rope_
{
	template <typename RT>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, node_info_t<RT> const& x) -> void
	{
		atma::memory_construct_at(xfer_dest(dest + acc_count), x);
		++acc_count;
	}

	template <typename RT>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, maybe_node_info_t<RT> const& x) -> void
	{
		if (x)
		{
			node_internal_construct_(acc_count, dest, *x);
		}
	}

	template <typename RT>
	inline auto node_internal_construct_(size_t& acc_count, node_info_t<RT>* dest, range_of_element_type<node_info_t<RT>> auto&& xs) -> void
	{
		for (auto&& x : xs)
		{
			node_internal_construct_(acc_count, dest, x);
		}
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
		for (auto const& x : children())
			result += x.characters;
		return result;
	}

	template <typename RT>
	inline auto node_internal_t<RT>::clone() const -> node_ptr<RT>
	{
		return make_internal_ptr<RT>(this->children());
	}

	template <typename RT>
	inline auto node_internal_t<RT>::calculate_combined_info() const -> text_info_t
	{
		return atma::foldl(children(), text_info_t(), functors::add);
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_leaf_t
//
//---------------------------------------------------------------------
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

	template <typename RT>
	inline auto node_leaf_t<RT>::clone() const -> node_ptr<RT>
	{
		return make_leaf_ptr<RT>(xfer_src(this->buf_));
	}

	template <typename RT>
	inline auto node_leaf_t<RT>::byte_idx_from_char_idx(text_info_t const& info, size_t char_idx) const -> size_t
	{
		return utf8_charseq_idx_to_byte_idx(this->buf.data() + info.dropped_bytes, info.bytes, char_idx);
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_t
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT>
	inline node_t<RT>::node_t(node_internal_t<RT> const& x)
		: variant_{x}
	{}

	template <typename RT>
	inline node_t<RT>::node_t(node_leaf_t<RT> const& x)
		: variant_{x}
	{}

	template <typename RT>
	inline bool node_t<RT>::is_internal() const
	{
		return variant_.index() == 0;
	}

	template <typename RT>
	inline bool node_t<RT>::is_leaf() const
	{
		return variant_.index() == 1;
	}

	template <typename RT>
	inline auto node_t<RT>::known_internal() -> node_internal_t<RT>&
	{
		return std::get<node_internal_t<RT>>(variant_);
	}

	template <typename RT>
	inline auto node_t<RT>::known_leaf() -> node_leaf_t<RT>&
	{
		return std::get<node_leaf_t<RT>>(variant_);
	}

	template <typename RT>
	inline auto node_t<RT>::clone() -> node_ptr<RT>
	{
		return this->visit(
			[&](node_internal_t<RT> const& x) { return x.clone(); },
			[&](node_leaf_t<RT> const& x) { return x.clone(); });
	}

	// visit node with functors
	template <typename RT>
	template <typename... Args>
	inline auto node_t<RT>::visit(Args&&... args)
	{
		return std::visit(visit_with{std::forward<Args>(args)...}, variant_);
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: text algorithms
//
//---------------------------------------------------------------------
namespace atma::_rope_
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
namespace atma::_rope_
{
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
namespace atma::_rope_
{
	// returns: <index-of-child-node, remaining-characters>
	template <typename RT>
	inline auto find_for_char_idx(node_internal_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		size_t child_idx = 0;
		size_t acc_chars = 0;
		for (auto const& child : x.children())
		{
			if (char_idx <= acc_chars + child.characters)
				break;

			acc_chars += child.characters;
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
namespace atma::_rope_
{
	template <typename RT>
	inline auto replace_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& repl_info) -> node_info_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "replace_: called on non-internal node");
		ATMA_ASSERT(idx < dest.children, "replace_: index out of bounds");

		auto& dest_node = dest.node->known_internal();
		auto children = dest_node.children();

		auto result_node = make_internal_ptr<RT>(
			xfer_src(children, idx),
			repl_info,
			xfer_src(children, idx + 1, children.size() - idx - 1));

		auto result = node_info_t<RT>{result_node};
		return result;
	}

	template <typename RT>
	inline auto append_(node_info_t<RT> const& dest, node_info_t<RT> const& insertee) -> node_info_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "append_: called on non-internal node");
		ATMA_ASSERT(dest.children < RT::branching_factor, "append_: insufficient space");

		auto& dest_node = dest.node->known_internal();
		auto children = dest_node.children();

		auto result_node = make_internal_ptr<RT>(
			xfer_src(children, dest.children),
			insertee);

		auto result = node_info_t<RT>{dest + static_cast<text_info_t const&>(insertee), result_node};
		return result;
	}

	template <typename RT>
	inline auto insert_(node_info_t<RT> const& dest, size_t idx, node_info_t<RT> const& ins_info) -> insert_result_t<RT>
	{
		ATMA_ASSERT(dest.node->is_internal(), "insert_: called on non-internal node");
		ATMA_ASSERT(idx <= dest.children, "insert_: index out of bounds");

		auto& dest_node = dest.node->known_internal();
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
	inline auto tree_merge_nodes_(tree_and_height_t<RT> const& left, tree_and_height_t<RT> const& right) -> tree_and_height_t<RT>
	{
		// validate assumptions:
		//  - both nodes are leaves, or both are internal
		//  - both nodes are at the same height
		ATMA_ASSERT(left.info.node->is_leaf() == right.info.node->is_leaf());
		ATMA_ASSERT(left.height == right.height);

		if constexpr (ATMA_ENABLE_ASSERTS)
		{
			node_info_t<RT> right_node_info_temp{right.info.node};
			ATMA_ASSERT(right_node_info_temp == right.info);
		}

		auto result_text_info = 
			static_cast<text_info_t const&>(left.info) + 
			static_cast<text_info_t const&>(right.info);

		if (left.info.node->is_leaf())
		{
			// we know the height is two because we've just added an internal
			// node above our two leaf nodes

			auto result_node = make_internal_ptr<RT>(left.info, right.info);
			return {node_info_t<RT>{result_text_info, result_node}, 2};
		}
		
		auto const& li = left.info.node->known_internal();
		auto const& ri = right.info.node->known_internal();

		if (li.children().size() >= RT::minimum_branches && ri.children().size() >= RT::minimum_branches)
		{
			// both nodes are saturated enough where we can just make a node above

			auto result_node = make_internal_ptr<RT>(left.info, right.info);
			return {node_info_t<RT>{result_text_info, result_node}, left.height + 1};
		}
		else if (size_t const children_count = li.children().size() + ri.children().size(); children_count <= RT::branching_factor)
		{
			// both nodes are so UNsaturated that we can merge the children into a new node

			auto result_node = make_internal_ptr<RT>(li.children(), ri.children());
			return {node_info_t<RT>{result_text_info, result_node}, left.height};
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
				xfer_src(li.children(), take_from_left_for_left),
				xfer_src(ri.children(), take_from_right_for_left));

			node_info_t<RT> new_left_info{new_left_node};

			auto new_right_node = make_internal_ptr<RT>(
				xfer_src(li.children()).last(take_from_left_for_right),
				xfer_src(ri.children()).last(take_from_right_for_right));

			node_info_t<RT> new_right_info{new_right_node};

			auto result_node = make_internal_ptr<RT>(new_left_info, new_right_info);


			if constexpr (ATMA_ENABLE_ASSERTS)
			{
				auto new_result_info = static_cast<text_info_t const&>(new_left_info) + static_cast<text_info_t const&>(new_right_info);
				ATMA_ASSERT(result_text_info == new_result_info);
			}

			return {node_info_t<RT>{result_text_info, result_node}, left.height + 1};
		}
	}


	template <typename RT>
	auto node_split_across_lhs_(tree_and_height_t<RT> const& tree, size_t idx) -> tree_and_height_t<RT>
	{
		if (idx == 0)
		{
			// no children I guess!
			return {make_leaf_ptr<RT>(), 1};
		}
		else if (idx == tree.info.children - 1)
		{
			// all children from first node
			return tree;
		}
		else if (idx == 1)
		{
			return {tree.info.node->known_internal().child_at(0), tree.height - 1};
		}
		else
		{
			return {make_internal_ptr<RT>(tree.info.node->known_internal().children().subspan(0, idx)), tree.height};
		}
	}

	template <typename RT>
	auto node_split_across_rhs_(tree_and_height_t<RT> const& tree, size_t idx) -> tree_and_height_t<RT>
	{
		if (idx == 0)
		{
			// all children from first node
			return tree;
		}
		else if (idx == tree.info.children - 1)
		{
			// no children I guess!
			return {make_leaf_ptr<RT>(), 1};
		}
		else if (idx == tree.info.children - 2)
		{
			return {tree.info.node->known_internal().child_at(tree.info.children - 1), tree.height - 1};
		}
		else
		{
			return {make_internal_ptr<RT>(tree.info.node->known_internal().children().subspan(idx + 1)), tree.height};
		}
	}

	template <typename RT>
	auto node_split_across_(tree_and_height_t<RT> const& tree, size_t idx) -> std::tuple<tree_and_height_t<RT>, tree_and_height_t<RT>>
	{
		return {node_split_across_lhs_(tree, idx), node_split_across_rhs_(tree, idx)};
	}


	template <typename RT>
	inline auto tree_concat_(tree_and_height_t<RT> const& left, tree_and_height_t<RT> const& right) -> tree_and_height_t<RT>
	{
		// validate assumptions:
		// - left & right are both valid trees
		ATMA_ASSERT(validate_rope_(left.info));
		ATMA_ASSERT(validate_rope_(right.info));


		if (left.height < right.height)
		{
			ATMA_ASSERT(right.height > 0);
			ATMA_ASSERT(right.info.node->is_internal());

			auto right_children = right.info.node->known_internal().children();

			bool const left_is_saturated = left.info.node->is_leaf() || left.info.children >= RT::branching_factor;
			bool const left_is_one_level_below_right = left.height + 1 == right.height;
			bool const right_has_space_for_another_child = right_children.size() < RT::branching_factor;

			if (left_is_saturated && left_is_one_level_below_right && right_has_space_for_another_child)
			{
				auto n = insert_(right.info, 0, left.info);
				ATMA_ASSERT(!n.maybe_rhs);
				return {n.lhs, right.height};
			}
			else if (left_is_saturated && left_is_one_level_below_right)
			{
				auto [ins_left, ins_right] = insert_(right.info, 0, left.info);
				ATMA_ASSERT(ins_right);
				return tree_merge_nodes_<RT>({ins_left, right.height}, {*ins_right, right.height});
			}
			else if (auto subtree = tree_concat_(left, {right_children.front(), right.height - 1}); subtree.height == left.height)
			{
				// if the subtree height is the same as the starting height, it means
				// that we were able to merge the children of the LHS & RHS fully into
				// one node, and we can just replace the front node of our larger tree
				// (which in this case is right)

				node_info_t<RT> result = replace_(right.info, 0, subtree.info);
				ATMA_ROPE_VERIFY_INFO_SYNC(result);
				return {result, left.height};

				//node_info_t<RT> result{make_internal_ptr<RT>(subtree.info, xfer_src(right_children).skip(1))};
				//return {result, right.height};
			}
			else if (subtree.height == right.height - 1)
			{
				// if our subtree is taller than left, it means we redristributed (or
				// didn't need to) our nodes into two, and placed a 'root' above them,
				// growing the tree. this means the new subtree is the right height to
				// merge with us (sans the front node which was merged already)

				auto n2 = make_internal_ptr<RT>(xfer_src(right_children).skip(1));
				return tree_concat_(subtree, {n2, right.height});
			}
			else if (subtree.height == right.height)
			{
				return subtree;
			}
			else
			{
				ATMA_ASSERT(false, "oh no");
				return {};
			}
		}
		else if (right.height < left.height)
		{
			ATMA_ASSERT(left.height > 0);
			ATMA_ASSERT(left.info.node->is_internal());

			auto left_children = left.info.node->known_internal().children();

			bool const right_is_saturated = right.info.node->is_leaf() || right.info.children >= RT::branching_factor;
			bool const right_is_one_level_below_left = right.height + 1 == left.height;
			bool const left_has_space_for_append = left_children.size() < RT::branching_factor;

			if (right_is_saturated && right_is_one_level_below_left && left_has_space_for_append)
			{
				auto n = append_(left.info, right.info);
				ATMA_ROPE_VERIFY_INFO_SYNC(n);
				return {n, left.height};
			}
			else if (right_is_saturated && right_is_one_level_below_left)
			{
				auto [ins_left, ins_right] = insert_(left.info, left.info.children, right.info);
				ATMA_ASSERT(ins_right);
				return tree_merge_nodes_<RT>({ins_left, left.height}, {*ins_right, left.height});
			}
			else if (auto subtree = tree_concat_({left_children.back(), left.height - 1}, right); subtree.height == right.height)
			{
				node_info_t<RT> result{replace_(left.info, left_children.size() - 1, subtree.info)};
				ATMA_ROPE_VERIFY_INFO_SYNC(result);
				return {result, left.height};
			}
			else if (subtree.height == left.height - 1)
			{
				auto left_prime = make_internal_ptr<RT>(xfer_src(left_children).drop(1));
				return tree_merge_nodes_({left_prime, left.height}, subtree);
			}
			else if (subtree.height == left.height)
			{
				// we successfully merged the whole tree

				return subtree;
			}
			else
			{
				ATMA_ASSERT(false, "oh no");
				return {};
			}
		}
		else
		{
			auto result = tree_merge_nodes_(left, right);
			ATMA_ROPE_VERIFY_INFO_SYNC(result.info);
			return result;
		}
	}













	template <typename RT, typename Data, typename Fp>
	inline auto navigate_upwards_passthrough_(node_info_t<RT> const&, node_internal_t<RT>&, size_t, navigate_to_leaf_result_t<RT, Data, Fp> const& x)
		-> navigate_to_leaf_result_t<RT, Data, Fp>
	{
		return x;
	}


	template <typename RT, typename Data, typename Fd, typename Fp, typename Fu>
	inline auto navigate_to_leaf(node_info_t<RT> const& info, Data data, Fd&& down_fn, Fp&& payload_fn, Fu&& up_fn) -> navigate_to_leaf_result_t<RT, Data, Fp>
	{
		// okay so this function seems massive and complex, but 90% of the code is constexpr bools determining
		// exactly the function-signatures of the user-supplied functions, so users don't have to place a bunch
		// of random args when they don't need them. the actual algorithm and real code is dead simple:
		//
		//  1) leaf nodes: call payload_fn and return that
		//  2) internal nodes:
		//     a) call down_fn to figure out which child to navigate to
		//     b) recurse onto that child
		//     c) with the result from our recursion, call up_fn and return that as the result
		//     d) fin.
		//
		using result_type = navigate_to_leaf_result_t<RT, Data, Fp>;

		constexpr bool const select_function_returns_child_idx =
			std::is_invocable_r_v<
				std::tuple<size_t>,
				Fd, node_internal_t<RT>&, Data>;

		constexpr bool const select_function_returns_child_idx_and_data =
			std::is_invocable_r_v<
				std::tuple<size_t, Data>,
				Fd, node_internal_t<RT>&, Data>;

		constexpr bool const select_function_returns_child_idx_and_info =
			std::is_invocable_r_v<
				std::tuple<size_t, node_info_t<RT>>,
				Fd, node_internal_t<RT>&, Data>;

		constexpr bool const select_function_returns_child_idx_and_info_and_data =
			std::is_invocable_r_v<
				std::tuple<size_t, node_info_t<RT>, Data>,
				Fd, node_internal_t<RT>&, Data>;

		constexpr bool const up_fn_takes_only_child_idx =
			std::is_invocable_v<
				Fu, node_info_t<RT> const&, node_internal_t<RT>&,
				size_t,
				result_type>;

		constexpr bool const up_fn_takes_only_child_idx_and_data =
			std::is_invocable_v<
				Fu, node_info_t<RT> const&, node_internal_t<RT>&,
				size_t, Data,
				result_type>;

		constexpr bool const up_fn_takes_both_child_idx_and_info =
			std::is_invocable_v<
				Fu, node_info_t<RT> const&, node_internal_t<RT>&,
				size_t, node_info_t<RT>&,
				result_type>;

		constexpr bool const up_fn_takes_both_child_idx_and_info_and_data =
			std::is_invocable_v<
				Fu, node_info_t<RT> const&, node_internal_t<RT>&,
				size_t, node_info_t<RT>&, Data,
				result_type>;

		auto const down_fn_prime = [&](node_internal_t<RT>& x, Data data)
		{
			if constexpr (select_function_returns_child_idx)
			{
				auto [child_idx] = std::invoke(std::forward<Fd>(down_fn), x, data);
				auto&& child_info = x.child_at(child_idx);
				return std::make_tuple(child_idx, std::ref(child_info), data);
			}
			else if constexpr (select_function_returns_child_idx_and_data)
			{
				auto [child_idx, data_prime] = std::invoke(std::forward<Fd>(down_fn), x, data);
				auto&& child_info = x.child_at(child_idx);
				return std::make_tuple(child_idx, std::ref(child_info), data_prime);
			}
			else if constexpr (select_function_returns_child_idx_and_info)
			{
				auto [child_idx, child_info] = std::invoke(std::forward<Fd>(down_fn), x, data);
				return std::make_tuple(child_idx, std::ref(child_info), data);
			}
			else if constexpr (select_function_returns_child_idx_and_info_and_data)
			{
				return std::invoke(std::forward<Fd>(down_fn), x, data);
			}
			else
			{
				static_assert(false, "bad select return-type");
			}
		};

		auto const up_fn_prime = [&](node_info_t<RT> const& info, node_internal_t<RT>& x, size_t child_idx, node_info_t<RT> const& child_info, Data data, result_type result)
		{
			if constexpr (up_fn_takes_only_child_idx)
				return std::invoke(std::forward<Fu>(up_fn), info, x, child_idx, result);
			else if constexpr (up_fn_takes_only_child_idx_and_data)
				return std::invoke(std::forward<Fu>(up_fn), info, x, child_idx, data, result);
			else if constexpr (up_fn_takes_both_child_idx_and_info)
				return std::invoke(std::forward<Fu>(up_fn), info, x, child_idx, child_info, result);
			else if constexpr (up_fn_takes_both_child_idx_and_info_and_data)
				return std::invoke(std::forward<Fu>(up_fn), info, x, child_idx, child_info, data, result);
			else
				static_assert(false, "bad up-function signature");
		};

		// yeah this is literally all the Real Code there actually is...
		return info.node->visit(
			[&](node_internal_t<RT>& x)
			{
				auto [child_idx, child_info, data_prime] = down_fn_prime(x, data);

				auto result = navigate_to_leaf(
					child_info, data_prime,
					std::forward<Fd>(down_fn),
					std::forward<Fp>(payload_fn),
					std::forward<Fu>(up_fn));

				return up_fn_prime(info, x, child_idx, child_info, data_prime, result);
			},
			[&](node_leaf_t<RT>& x)
			{
				if constexpr (std::is_invocable_v<Fp, decltype(info), decltype(x)>)
					return std::invoke(std::forward<Fp>(payload_fn), info, x);
				else if constexpr (std::is_invocable_v<Fp, decltype(info), decltype(x), decltype(data)>)
					return std::invoke(std::forward<Fp>(payload_fn), info, x, data);
				else
					static_assert(false, "bad function signature for payload-function");
			});
	}

	template <typename RT, typename Fp, typename Fu>
	inline auto navigate_to_front_leaf(node_info_t<RT> const& info, Fp&& payload_fn, Fu&& up_fn)
	{
		return navigate_to_leaf(info, size_t(),
			[](node_internal_t<RT>& x, size_t) { return std::make_tuple(0); },
			std::forward<Fp>(payload_fn),
			std::forward<Fu>(up_fn));
	}

	template <typename RT, typename Fp>
	inline auto navigate_to_front_leaf(node_info_t<RT> const& info, Fp&& payload_fn)
	{
		return navigate_to_front_leaf(info,
			std::forward<Fp>(payload_fn),
			&navigate_upwards_passthrough_<RT, size_t, Fp>);
	}

	template <typename RT, typename Fp, typename Fu>
	auto navigate_to_back_leaf(node_info_t<RT> const& info, Fp&& payload_fn, Fu&& up_fn)
	{
		return navigate_to_leaf(info, size_t(),
			[](node_internal_t<RT>& x, size_t) { return std::make_tuple(x.children().size() - 1); },
			std::forward<Fp>(payload_fn),
			std::forward<Fu>(up_fn));
	}
}


//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: text-tree functions
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	// edit_chunk_at_char

	template <typename RT, typename F>
	inline auto edit_chunk_at_char(node_info_t<RT> const& info, size_t char_idx, F&& payload_function) -> edit_result_t<RT>
	{
		return navigate_to_leaf(info, char_idx,
			find_for_char_idx<RT>,
			std::forward<F>(payload_function),
			stitch_upwards_<RT>);
	}
}


namespace atma::_rope_
{
	template <typename RT>
	inline auto stitch_upwards_simple_(node_info_t<RT> const& info, node_internal_t<RT>& node, size_t child_idx, maybe_node_info_t<RT> const& child_info) -> maybe_node_info_t<RT>
	{
		return (child_info)
			? maybe_node_info_t<RT>{ replace_(info, child_idx, *child_info) }
			: maybe_node_info_t<RT>{};
	}

	template <typename RT>
	inline auto stitch_upwards_(node_info_t<RT> const& info, node_internal_t<RT>& node, size_t child_idx, edit_result_t<RT> const& er) -> edit_result_t<RT>
	{
		auto const& [left_info, maybe_right_info, seam] = er;

		// validate that there's no seam between our two edit-result nodes. this
		// should have been caught in whatever edit operation was performed. stitching
		// the tree and melding crlf pairs is for siblings
		if (maybe_right_info && left_info.node->is_leaf())
		{
			auto const& leftleaf = left_info.node->known_leaf();
			auto const& rightleaf = maybe_right_info->node->known_leaf();

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
				xfer_src(node.children().first(child_idx - 1)),
				mended_children[0], mended_children[1], mended_children[2], mended_children[3],
				xfer_src(node.children().subspan(child_idx + 1)))
			: make_internal_ptr<RT>(
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

			auto sibling_node_prime = navigate_to_back_leaf(*sibling_node,
				append_lf_<RT>,
				stitch_upwards_simple_<RT>);

			if (sibling_node_prime)
			{
				// drop lf from seam node
				auto seam_node_prime = navigate_to_back_leaf(seam_node,
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

			auto sibling_node_prime = navigate_to_front_leaf(*sibling_node,
				drop_lf_<RT>,
				stitch_upwards_simple_<RT>);

			if (sibling_node_prime)
			{
				// append lf to seamed leaf
				auto seam_node_prime = navigate_to_back_leaf(seam_node,
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

namespace atma::_rope_
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
			return edit_chunk_at_char(dest, char_idx, [&](auto&& a, auto&& b, auto&& c) { return insert_small_text_<RT>(a, b, c, insbuf); });
		}
	}

	template <typename RT>
	inline auto split_payload_(node_info_t<RT> const& leaf_info, node_leaf_t<RT> const& leaf, std::tuple<size_t, size_t> char_idx_and_height) -> split_result_t<RT>
	{
		auto const& [char_idx, height] = char_idx_and_height;

		auto byte_idx = leaf.byte_idx_from_char_idx(leaf_info, char_idx);
		auto split_idx = leaf_info.dropped_bytes + byte_idx;

		if (byte_idx == leaf_info.dropped_bytes)
		{
			// we are asking to split directly at the front of our buffer.
			// so... we're not doing that. we're instead return a no-result
			// for the lhs, and for the rhs we can just pass along our
			// existing node, as it does not need to be modified

			return {1, make_leaf_ptr<RT>(), 1, leaf_info, 1};
		}
		else if (byte_idx == leaf_info.dropped_bytes + leaf_info.bytes)
		{
			// in this case, we are asking to split at the end of our buffer.
			// let's instead return a no-result for the rhs, and pass this
			// node on the lhs.

			return {1, leaf_info, 1, make_leaf_ptr<RT>(), 1};
		}
		else
		{
			// okay, our char-idx has landed us in the middle of our buffer.
			// split the buffer into two and return both nodes.

			auto left = make_leaf_ptr<RT>(xfer_src(leaf.buf.data() + leaf_info.dropped_bytes, byte_idx));
			auto right = make_leaf_ptr<RT>(xfer_src(leaf.buf.data() + split_idx, leaf.buf.size() - split_idx));

			return {1, left, 1, right, 1};
		}
	}

	template <typename RT>
	inline auto split_up_fn_(node_info_t<RT> const& info, node_internal_t<RT> const& node, size_t split_idx, split_result_t<RT> const& result) -> split_result_t<RT>
	{
		auto const& [orig_height, split_left, split_right] = result;
		
		// "we", in this stack-frame, are one level higher than what was returned in result
		size_t const our_height = orig_height + 1;

		// validate assumptions:
		//  - split_idx is valid
		//  - either @left, @right, or both are valid
		ATMA_ASSERT(split_idx < info.children);
		//ATMA_ASSERT(split_left.info || split_right.info);

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
#if 0
		{
			if (split_left.info && (*split_left.info).node->is_internal())
			{
				auto const& lin = split_left.info->node->known_internal();
				for (auto const& x : lin.children())
				{
					ATMA_ASSERT(validate_rope_.internal_node(x));
				}
			}

			if (split_right.info && (*split_right.info).node->is_internal())
			{
				auto const& lin = split_right.info->node->known_internal();
				for (auto const& x : lin.children())
				{
					ATMA_ASSERT(validate_rope_.internal_node(x));
				}
			}
		}
#endif
		// let's assume that all children of our children (if they exist) are valid.
		[[maybe_unused]] bool const is_left_valid = !split_left.info.node->is_internal()
			|| split_left.info.node->known_internal().children().size() >= RT::minimum_branches;

		[[maybe_unused]] bool const is_right_valid = !split_right.info.node->is_internal()
			|| split_right.info.node->known_internal().children().size() >= RT::minimum_branches;

		
		if (split_idx == 0)
		{
			if (is_right_valid && split_right.height == orig_height)
			{
				node_info_t<RT> right{replace_(info, 0, split_right.info)};
				return {our_height, split_left, {right, split_right.height + 1}};
			}
			else
			{
				auto rhs_children = node_split_across_rhs_<RT>(
					{info, our_height},
					split_idx);

				auto right = tree_concat_<RT>(
					split_right,
					rhs_children);

				return {our_height, split_left, right};
			}
		}
		else if (split_idx == info.children - 1)
		{
			if (is_left_valid && split_left.height == orig_height)
			{
				node_info_t<RT> left{replace_(info, split_idx, split_left.info)};
				return {our_height, {left, split_left.height + 1}, split_right};
			}
			else
			{
				auto lhs_children = node_split_across_lhs_<RT>(
					{info, our_height},
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
				{info, our_height},
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
	inline auto find_for_char_idx_acc_height(node_internal_t<RT> const& x, std::tuple<size_t, size_t> char_idx_and_height) -> std::tuple<size_t, std::tuple<size_t, size_t>>
	{
		auto [idx, char_idx] = find_for_char_idx(x, std::get<0>(char_idx_and_height));

		return {idx, {char_idx, std::get<1>(char_idx_and_height) + 1}};
	}

	template <typename RT>
	inline auto split(node_info_t<RT> const& dest, size_t char_idx) -> split_result_t<RT>
	{
		return navigate_to_leaf(dest, std::make_tuple(char_idx, size_t(1)),
			&find_for_char_idx_acc_height<RT>,
			&split_payload_<RT>,
			&split_up_fn_<RT>);
	}
}

namespace atma::_rope_
{
	// insert_small_text_

	template <typename RT>
	inline auto insert_small_text_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t char_idx, src_buf_t insbuf) -> edit_result_t<RT>
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

#if 0
	// todo: this function is something like the function required to insert a few leaves
	// worth of text all at once in the most optimal manner
	template <typename RT>
	inline auto insert_small_text_and_more_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t char_idx, src_buf_t insbuf, std::vector<node_ptr<RT>> const& more_nodes)
		-> std::vector<node_info_t<RT>>
	{
		// calculate byte index from character index
		auto byte_idx = leaf.byte_idx_from_char_idx(leaf_info, char_idx);


		auto [lhs_info, rhs_info] = _rope_::insert_and_redistribute<RT>(leaf_info, xfer_src(leaf.buf), insbuf, byte_idx);
	}
#endif
}

namespace atma::_rope_
{
#define ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(info, buf) \
	do { \
		ATMA_ASSERT((info).dropped_bytes + (info).bytes == (buf).size()); \
	} while(0)


	template <typename RT>
	inline auto drop_lf_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t) -> maybe_node_info_t<RT>
	{
		ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(leaf_info, leaf.buf);

		if (leaf.buf[leaf_info.dropped_bytes] == charcodes::lf)
		{
			ATMA_ASSERT(leaf_info.line_breaks > 0);

			auto result = leaf_info
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
	inline auto append_lf_(node_info_t<RT> const& leaf_info, node_leaf_t<RT>& leaf, size_t) -> maybe_node_info_t<RT>
	{
		ATMA_ROPE_ASSERT_LEAF_INFO_AND_BUF_IN_SYNC(leaf_info, leaf.buf);
		
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
		bool const has_trailing_cr = !leaf.buf.empty() && leaf.buf.back() == charcodes::cr;
		if (!has_trailing_cr)
		{
			return {};
		}

		bool const can_fit_in_chunk = leaf.buf.size() + 1 <= RT::buf_size;
		ATMA_ASSERT(can_fit_in_chunk);

		leaf.buf.push_back(charcodes::lf);
		auto result_info = leaf_info + text_info_t{.bytes = 1, .characters = 1};
		return result_info;
	}
}














































































namespace atma::_rope_
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








namespace atma::_rope_
{
	template <typename T>
	constexpr auto ceil_div(T x, T y)
	{
		return (x + y - 1) / y;
	}

	template <typename RT>
	struct build_rope_t_
	{
		inline static constexpr size_t max_level_size = RT::branching_factor + ceil_div<size_t>(RT::branching_factor, 2u);

		using level_t = std::tuple<size_t, std::array<node_info_t<RT>, max_level_size>>;
		using level_stack_t = std::list<level_t>;

		auto stack_level(level_stack_t& stack, size_t level) const -> level_t&;
		decltype(auto) stack_level_size(level_stack_t& stack, size_t level) const { return std::get<0>(stack_level(stack, level)); }
		decltype(auto) stack_level_array(level_stack_t& stack, size_t level) const { return std::get<1>(stack_level(stack, level)); }
		decltype(auto) stack_level_size(level_t const& x) const { return std::get<0>(x); }
		decltype(auto) stack_level_array(level_t const& x) const { return std::get<1>(x); }

		void stack_push(level_stack_t& stack, int level, node_info_t<RT> const& insertee) const;
		auto stack_finish(level_stack_t& stack) const -> node_info_t<RT>;

		auto operator ()(src_buf_t str) const -> basic_rope_t<RT>;
	};

	template <typename RT>
	inline constexpr build_rope_t_<RT> build_rope_;
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto build_rope_t_<RT>::stack_level(level_stack_t& stack, size_t level) const -> level_t&
	{
		auto i = stack.begin();
		std::advance(i, level);
		return *i;
	}

	template <typename RT>
	inline void build_rope_t_<RT>::stack_push(level_stack_t& stack, int level, node_info_t<RT> const& insertee) const
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
			node_info_t<RT> higher_level_node{make_internal_ptr<RT>(
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
		int i = 0;
		for (auto level_iter = stack.begin(); level_iter != stack.end(); ++level_iter, ++i)
		{
			auto const level_size = stack_level_size(*level_iter);
			auto const& level_array = stack_level_array(*level_iter);

			if (i == stack.size() - 1 && level_size == 1)
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
					xfer_src(level_array.data(), level_size))};

				stack_push(stack, i + 1, higher_level_node);
			}
			else
			{
				// we have too many nodes to roll up into one internal node, so
				// split the remaining nodes into two, making sure to balance
				// them as to not invalidate the invariants of the btree

				auto const lhs_size = ceil_div<size_t>(level_size, 2);
				auto const rhs_size = level_size - lhs_size;

				node_info_t<RT> higher_level_lhs_node{make_internal_ptr<RT>(
					xfer_src(level_array.data(), lhs_size))};

				node_info_t<RT> higher_level_rhs_node{make_internal_ptr<RT>(
					xfer_src(level_array.data() + lhs_size, rhs_size))};

				stack_push(stack, i + 1, higher_level_lhs_node);
				stack_push(stack, i + 1, higher_level_rhs_node);
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
		basic_rope_t<RT> result;
		result.root() = r;
		return result;
	}
}

























































namespace atma::_rope_
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

namespace atma::_rope_
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
				return node_info_t<RT>{make_internal_ptr<RT>(lhs, *rhs)};
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












































namespace atma
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

		if (edit_result.right.has_value())
		{
			(_rope_::text_info_t&)root_ = edit_result.left + *edit_result.right;
			root_.node = _rope_::make_internal_ptr<RT>(edit_result.left, *edit_result.right);
		}
		else
		{
			root_ = edit_result.left;
		}
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::split(size_t char_idx) const -> std::tuple<basic_rope_t<RT>, basic_rope_t<RT>>
	{
		auto [depth, left, right] = _rope_::split(root_, char_idx);
		return { basic_rope_t{left.info}, basic_rope_t{right.info} };
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
#endif

