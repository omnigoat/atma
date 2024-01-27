//module;

#include <atma/assert.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>

#include <optional>
#include <array>
#include <functional>
#include <utility>
#include <span>
#include <concepts>
#include <list>
#include <ranges>
#include <variant>
#include <ranges>

#define ATMA_ROPE_DEBUG_BUFFER 1

//export module atma.rope;

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
namespace atma::_rope_
{
	struct text_info_t;

	template <typename> struct node_internal_t;
	template <typename> struct node_leaf_t;
	template <typename> struct node_t;

	template <typename RT> using node_ptr = intrusive_ptr<node_t<RT>>;
	template <typename RT> using node_internal_ptr = intrusive_ptr<node_internal_t<RT>>;
	template <typename RT> using node_leaf_ptr = intrusive_ptr<node_leaf_t<RT>>;

	template <typename RT> struct tree_t;
	template <typename RT> struct tree_branch_t;
	template <typename RT> struct tree_leaf_t;

	template <typename RT> using maybe_tree_t = std::optional<tree_t<RT>>;
	template <typename RT> using maybe_tree_branch_t = std::optional<tree_branch_t<RT>>;

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




	// monadics
	template <typename RT>
	auto maybe_branch_to_maybe_tree(maybe_tree_branch_t<RT> const& x) -> maybe_tree_t<RT>
	{
		return (x.has_value())
			? maybe_tree_t<RT>{x.value().as_tree()}
			: maybe_tree_t<RT>{std::nullopt};
	}
}


//
// traits
//
namespace atma
{
	template <size_t BranchingFactor, size_t BufferSize, bool Debug = false>
	struct rope_basic_traits
	{
		// how many children an internal node has
		constexpr static size_t const branching_factor = BranchingFactor;
		constexpr static size_t const minimum_branches = branching_factor / 2;
		constexpr static size_t const branches_in_half = _rope_::ceil_div(branching_factor, 2ull);

		// rope-buffer size
		constexpr static size_t const buf_size = BufferSize;
		
		constexpr static size_t buf_edit_max_size = buf_size - 2;
		constexpr static size_t buf_edit_split_size = (buf_size / 2) - (buf_size / 32);
		constexpr static size_t buf_edit_split_drift_size = (buf_size / 32);

		constexpr static bool const debug_internal_validation = Debug;
	};

	using rope_default_traits = rope_basic_traits<4, 512>;
	using rope_test_traits = rope_basic_traits<4, 9, true>;
}

namespace atma::_rope_
{
	template <typename RT, typename = std::void_t<>>
	struct debug_internal_validation_t
		: std::false_type
	{};

	template <typename RT>
	struct debug_internal_validation_t<RT, std::void_t<decltype(RT::debug_internal_validation)>>
		: std::integral_constant<bool, RT::debug_internal_validation>
	{};

	template <typename RT>
	inline constexpr bool debug_internal_validation_v = debug_internal_validation_t<RT>::value;
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





namespace atma::_rope_
{
	template <typename RT> struct tree_t;
	template <typename RT> struct tree_branch_t;
	template <typename RT> struct tree_leaf_t;

	template <typename RT>
	struct tree_t : protected text_info_t
	{
		tree_t() = default;
		tree_t(node_ptr<RT> const&);
		tree_t(text_info_t const&, uint32_t children, node_ptr<RT> const&);
		tree_t(text_info_t const&, node_ptr<RT> const&);

		auto info() const -> text_info_t const&;
		auto child_count() const -> uint32_t;
		auto node() const -> node_t<RT> const&;
		auto node_pointer() const -> node_ptr<RT> const&;

		auto height() const -> uint32_t;
		auto size_chars() const -> size_t;
		auto size_bytes() const -> size_t;

		auto is_saturated() const;

		auto children() const;
		auto backing_children() const;

		auto is_branch() const -> bool;
		auto is_leaf() const -> bool;

		auto as_tree() const -> tree_t<RT> const& { return static_cast<tree_branch_t<RT> const&>(*this); }
		auto as_branch() const -> tree_branch_t<RT> const& { return static_cast<tree_branch_t<RT> const&>(*this); }
		auto as_leaf() const -> tree_leaf_t<RT> const& { return static_cast<tree_leaf_t<RT> const&>(*this); }

	protected:
		uint32_t child_count_ = 0;
		node_ptr<RT> node_;
	};

	template <typename RT>
	inline bool operator == (tree_t<RT> const& lhs, tree_t<RT> const& rhs)
	{
		return lhs.info() == rhs.info()
			&& lhs.child_count() == rhs.child_count()
			&& lhs.node_pointer() == rhs.node_pointer();
	}

	// deduction guides
	template <typename RT>
	tree_t(node_ptr<RT> const&) -> tree_t<RT>;
}

namespace atma::_rope_
{
	template <typename RT>
	struct tree_branch_t : tree_t<RT>
	{
		using tree_t<RT>::tree_t;

		auto node() const -> node_internal_t<RT> const&;

		// children as viewed by info
		auto children() const;
		auto child_at(int idx) const -> tree_t<RT> const&;

		auto backing_children() const;

		auto append(tree_t<RT> const& x) -> tree_branch_t<RT>;
	};
}

namespace atma::_rope_
{
	template <typename RT>
	struct tree_leaf_t : tree_t<RT>
	{
		using tree_t<RT>::tree_t;

		// conversion operators
		operator tree_t<RT>&();
		operator tree_t<RT> const&() const;

		auto node() const -> node_leaf_t<RT> const&;
		auto height() const;

		auto data() const -> src_buf_t;
		auto byte_idx_from_char_idx(size_t char_idx) const -> size_t;
	};
}














namespace atma::_rope_
{
	enum class seam_t
	{
		none = 0,
		left = 1,
		right = 2
	};

	inline seam_t operator & (seam_t lhs, seam_t rhs) { return seam_t{static_cast<int>(lhs) & static_cast<int>(rhs)}; }
	inline seam_t operator | (seam_t lhs, seam_t rhs) { return seam_t{static_cast<int>(lhs) | static_cast<int>(rhs)}; }
}

//
// treeop_result_t / edit_result_t
//
namespace atma::_rope_
{
	// result for tree operations (no leaves can be returned)
	template <typename RT>
	struct treeop_result_t
	{
		tree_branch_t<RT> left;
		maybe_tree_branch_t<RT> right;
	};

	// result for tree operations that result from text-operations
	template <typename RT>
	struct edit_result_t
	{
		edit_result_t() = default;

		edit_result_t(tree_t<RT> left, maybe_tree_t<RT> right = {}, seam_t seam = seam_t::none)
			: left{left}
			, right{right}
			, seam(seam)
		{}

		tree_t<RT> left;
		maybe_tree_t<RT> right;
		seam_t seam = seam_t::none;
	};
}


//
// node_t
//
namespace atma::_rope_
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

		constexpr bool is_branch() const;
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
		sizeof atma::ref_counted_of<node_t<rope_test_traits>> +sizeof(uint32_t)>,
		"node_t should be 64 bits big");
}



//
//  node_internal_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_internal_t : node_t<RT>
	{
		using children_type = std::span<tree_t<RT> const>;
		using size_type = uint32_t;

		// accepts in-order arguments to fill children_
		template <typename... Args>
		node_internal_t(uint32_t height, Args&&...);

		auto length() const -> size_t;

		auto child_at(size_type idx) const -> tree_t<RT> const&;
		auto child_at(size_type idx) -> tree_t<RT>&;

		auto try_child_at(size_type idx) -> maybe_tree_t<RT>;

		static constexpr size_t all_children = ~size_t();

		auto append(tree_t<RT> const& info);

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
		std::array<tree_t<RT>, RT::branching_factor> children_;
	};

}


//
// node_leaf_t
//
namespace atma::_rope_
{
	template <typename RT>
	struct node_leaf_t : node_t<RT>
	{
		template <typename... Args>
		node_leaf_t(Args&&...);

		using buf_t = charbuf_t<RT::buf_size>;

		// this buffer can only ever be appended to. nodes store how many
		// characters/bytes they address inside this buffer, so we can append
		// more data to this buffer and maintain an immutable data-structure
		buf_t buf;
	};
}



//
// make_internal_ptr / make_leaf_ptr
//
namespace atma::_rope_
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
//  rope
//
//---------------------------------------------------------------------
namespace atma
{
	template <typename RopeTraits>
	struct basic_rope_t
	{
		basic_rope_t();
		basic_rope_t(char const*, size_t);

		auto push_back(char const*, size_t) -> void;
		auto insert(size_t char_idx, char const* str, size_t sz) -> void;

		auto erase(size_t char_idx, size_t size_in_chars) -> void;

		auto split(size_t char_idx) const -> std::tuple<basic_rope_t<RopeTraits>, basic_rope_t<RopeTraits>>;

		template <typename F>
		auto for_all_text(F&& f) const;

		decltype(auto) root() const { return (root_); }

		// size in characters
		auto size() const -> size_t { return root_.info().characters; }
		auto size_bytes() const -> size_t { return root_.info().bytes - root_.info().dropped_bytes; }

		


	private:
		basic_rope_t(_rope_::tree_t<RopeTraits> const&);

	private:
		_rope_::tree_t<RopeTraits> root_;

		friend struct _rope_::build_rope_t_<RopeTraits>;
	};


	// comparison operators
	template <typename RT>
	auto operator == (basic_rope_t<RT> const&, basic_rope_t<RT> const&) -> bool;

	template <typename RT>
	auto operator == (basic_rope_t<RT> const&, std::string_view) -> bool;
	
	template <typename RT, std::ranges::contiguous_range Range>
	requires std::same_as<std::remove_const_t<std::ranges::range_value_t<Range>>, char>
	auto operator == (basic_rope_t<RT> const&, Range const& rhs) -> bool;
}


namespace atma
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
		auto operator  *() const -> tree_leaf_t<RT> const&;
		auto operator ->() const -> tree_leaf_t<RT> const*;

	private:
		atma::vector<std::tuple<tree_branch_t<RT>, int>> tree_stack_;

		template <typename RT2>
		friend auto operator == (basic_leaf_iterator_t<RT2> const& lhs, basic_leaf_iterator_t<RT2> const& rhs) -> bool;
	};
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

	template <typename RT>
	auto is_saturated(tree_t<RT> const&) -> bool;
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

	template <typename RT>
	auto tree_find_for_char_idx_within(tree_branch_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>;

	// find_for_char_idx_within :: returns <index-of-child-node, remaining-characters>
	//    this version will return a child that terminates on the index
	template <typename RT>
	auto find_for_char_idx_within(node_internal_t<RT> const&, size_t char_idx) -> std::tuple<size_t, size_t>;

	// for_all_text :: visits each leaf in sequence and invokes f(std::string_view)
	template <typename F, typename RT>
	auto for_all_text(F f, tree_t<RT> const& ri) -> void;
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
	auto replace_(tree_t<RT> const& dest, size_t idx, tree_t<RT> const& replacee)
		-> tree_branch_t<RT>;

	template <typename RT>
	auto append_(tree_t<RT> const& dest, tree_t<RT> const& insertee)
		-> tree_t<RT>;

#if 0
	template <typename RT>
	auto insert_(tree_t<RT> const& dest, size_t idx, tree_t<RT> const& insertee)
		-> treeop_result_t<RT>;
#endif

	template <typename RT>
	auto replace_and_insert_(tree_branch_t<RT> const& dest, size_t idx, tree_t<RT> const& repl_info, maybe_tree_t<RT> const& maybe_ins_info)
		-> treeop_result_t<RT>;
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
//---------------------------------------------------------------------
namespace atma::_rope_
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
	auto tree_concat_(tree_t<RT> const& tree, size_t tree_height, tree_t<RT> const& prepend_tree, size_t prepend_depth)
		-> tree_t<RT>;


	// stitching

	template <typename RT>
	auto stitch_upwards_simple_(tree_branch_t<RT> const&, size_t child_idx, maybe_tree_t<RT> const&)
		-> maybe_tree_t<RT>;

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
namespace atma::_rope_
{
	template <typename RT, typename F>
	auto edit_chunk_at_char(tree_t<RT> const& tree, size_t char_idx, F&& payload_function)
		-> edit_result_t<RT>;
}

namespace atma::_rope_
{
	// mending

	template <typename RT>
	using mend_result_type = std::optional<std::tuple<tree_t<RT>, tree_t<RT>>>;

	template <typename RT>
	auto mend_left_seam_(seam_t, tree_t<RT> const& sibling_node, tree_t<RT> const& seam_node)
		-> mend_result_type<RT>;

	template <typename RT>
	auto mend_right_seam_(seam_t, tree_t<RT> const& seam_node, tree_t<RT> const& sibling_node)
		-> mend_result_type<RT>;
}

namespace atma::_rope_
{
	// insert

	template <typename RT>
	auto insert(size_t char_idx, tree_t<RT> const& dest, src_buf_t const& insbuf)
		-> edit_result_t<RT>;
}

namespace atma::_rope_
{
	// erase
	template <typename RT>
	struct erase_result_t
	{
		maybe_tree_t<RT> lhs, rhs;
	};

	template <typename RT>
	auto erase(tree_t<RT> const&, size_t char_idx, size_t size_in_chars)
		-> erase_result_t<RT>;

	template <typename RT>
	auto erase_(tree_t<RT> const&, size_t rel_char_idx, size_t rel_size_in_chars)
		-> erase_result_t<RT>;
}

namespace atma::_rope_
{
	// split

	template <typename RT>
	struct split_result_t
	{
		uint32_t tree_height = 0;
		maybe_tree_t<RT> left, right;
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

namespace atma::_rope_
{
	// insert_small_text_

	template <typename RT>
	auto insert_small_text_(tree_leaf_t<RT> const&, size_t char_idx, src_buf_t insbuf)
		-> edit_result_t<RT>;
}

namespace atma::_rope_
{
	// drop_lf_

	template <typename RT>
	auto drop_lf_(tree_leaf_t<RT> const&, size_t char_idx)
		-> maybe_tree_t<RT>;

	// append_lf_

	template <typename RT>
	auto append_lf_(tree_leaf_t<RT> const&, size_t char_idx)
		-> maybe_tree_t<RT>;
}










//
// validate_rope
//
namespace atma::_rope_
{
	struct validate_rope_t_
	{
	private:
		using check_node_result_type = std::tuple<bool, int>;

		template <typename RT>
		static auto check_node(tree_t<RT> const& info, size_t min_children = RT::minimum_branches)
			-> check_node_result_type;

	public:
		template <typename RT>
		auto operator ()(tree_t<RT> const& x) const -> bool;

		template <typename RT>
		auto internal_node(tree_t<RT> const& x) const -> bool;
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
		
		utf8_char_t prev_char;
		for (auto x : utf8_const_range_t{str, str + sz})
		{
			r.bytes += (uint16_t)x.size_bytes();
			++r.characters;

			// only count distinct newlines, so '\r\n' counts as only one
			bool const is_followon_lf = (x == '\n') && (prev_char == '\r');
			if (utf8_char_is_newline(x) && !is_followon_lf)
			{
				++r.line_breaks;
			}
			
			prev_char = x;
		}

		return r;
	}
}

//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: tree_t
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT>
	inline tree_t<RT>::tree_t(node_ptr<RT> const& node)
		: text_info_t{text_info_of(node)}
		, child_count_(valid_children_count(node))
		, node_(node)
	{}

	template <typename RT>
	inline tree_t<RT>::tree_t(text_info_t const& info, uint32_t children, node_ptr<RT> const& node)
		: text_info_t(info)
		, child_count_(children)
		, node_(node)
	{}

	template <typename RT>
	inline tree_t<RT>::tree_t(text_info_t const& info, node_ptr<RT> const& node)
		: text_info_t(info)
		, child_count_(valid_children_count(node))
		, node_(node)
	{}

	template <typename RT>
	inline auto tree_t<RT>::info() const -> text_info_t const&
	{
		return static_cast<text_info_t const&>(*this);
	}

	template <typename RT>
	inline auto tree_t<RT>::child_count() const -> uint32_t
	{
		return child_count_;
	}

	template <typename RT>
	inline auto tree_t<RT>::node() const -> node_t<RT> const&
	{
		return *node_;
	}

	template <typename RT>
	inline auto tree_t<RT>::node_pointer() const -> node_ptr<RT> const&
	{
		return node_;
	}

	template <typename RT>
	inline auto tree_t<RT>::height() const -> uint32_t
	{
		return this->node().height();
	}

	template <typename RT>
	inline auto tree_t<RT>::size_chars() const -> size_t
	{
		return this->info().characters - this->info().dropped_characters;
	}
	
	template <typename RT>
	inline auto tree_t<RT>::size_bytes() const -> size_t
	{
		return this->info().bytes - this->info().dropped_bytes;
	}

	template <typename RT>
	inline auto tree_t<RT>::is_saturated() const
	{
		return this->child_count_ >= RT::branching_factor / 2;
	}

	// children as viewed by info
	template <typename RT>
	inline auto tree_t<RT>::children() const
	{
		return node().visit(
			[&](node_internal_t<RT> const& x) -> node_internal_t<RT>::children_type { return x.children(child_count()); },
			[](node_leaf_t<RT> const&) -> node_internal_t<RT>::children_type { return typename node_internal_t<RT>::children_type{}; });
	}

	template <typename RT>
	inline auto tree_t<RT>::backing_children() const
	{
		return node().visit(
			[](node_internal_t<RT> const& x) { return x.children(); },
			[](node_leaf_t<RT> const&) { return typename node_internal_t<RT>::children_type{}; });
	}
	
	template <typename RT>
	inline auto tree_t<RT>::is_branch() const -> bool
	{
		return node().is_branch();
	}

	template <typename RT>
	inline auto tree_t<RT>::is_leaf() const -> bool
	{
		return node().is_leaf();
	}
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto tree_branch_t<RT>::node() const -> node_internal_t<RT> const&
	{
		return static_cast<node_internal_t<RT> const&>(this->tree_t<RT>::node());
	}

	// children as viewed by info
	template <typename RT>
	inline auto tree_branch_t<RT>::children() const
	{
		return this->node().children(this->child_count());
	}

	template <typename RT>
	inline auto tree_branch_t<RT>::child_at(int idx) const -> tree_t<RT> const&
	{
		return this->node().children()[idx];
	}

	template <typename RT>
	inline auto tree_branch_t<RT>::backing_children() const
	{
		return this->node().children();
	}

	template <typename RT>
	inline auto tree_branch_t<RT>::append(tree_t<RT> const& x) -> tree_branch_t<RT>
	{
		this->node_->as_branch().append(x);
		auto r = tree_branch_t<RT>{this->info() + x.info(), this->child_count_ + 1, this->node_};
		return r;
	}
}

//---------------------------------------------------------------------
//
//  tree_leaf_t
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT>
	inline tree_leaf_t<RT>::operator tree_t<RT>& ()
	{
		return *static_cast<tree_t<RT>*>(this);
	}
	
	template <typename RT>
	inline tree_leaf_t<RT>::operator tree_t<RT> const& () const
	{
		return *static_cast<tree_t<RT> const*>(this);
	}

	template <typename RT>
	inline auto tree_leaf_t<RT>::node() const -> node_leaf_t<RT> const&
	{
		return static_cast<node_leaf_t<RT> const&>(this->tree_t<RT>::node());
	}

	template <typename RT>
	inline auto tree_leaf_t<RT>::height() const
	{
		return 1;
	}

	template <typename RT>
	inline auto tree_leaf_t<RT>::data() const -> src_buf_t
	{
		return xfer_src(this->node().buf.data() + this->info().dropped_bytes, this->info().bytes);
	}

	template <typename RT>
	inline auto tree_leaf_t<RT>::byte_idx_from_char_idx(size_t char_idx) const -> size_t
	{
		return utf8_charseq_idx_to_byte_idx(this->node().buf.data() + this->info().dropped_bytes, this->info().bytes, char_idx);
	}
}




//---------------------------------------------------------------------
//
//  IMPLEMENTATION :: node_internal_t
//
//---------------------------------------------------------------------
namespace atma::_rope_
{
	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, tree_t<RT>* dest, tree_t<RT> const& x) -> void
	{
		atma::memory_construct_at(xfer_dest(dest + acc_count), x);
		++acc_count;
	}

	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, tree_t<RT>* dest, maybe_tree_t<RT> const& x) -> void
	{
		if (x)
		{
			node_internal_construct_(acc_count, dest, *x);
		}
	}

	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, tree_t<RT>* dest, range_of_element_type<tree_t<RT>> auto&& xs) -> void
	{
		for (auto&& x : xs)
		{
			node_internal_construct_(acc_count, dest, x);
		}
	}


	template <typename RT>
	inline auto node_internal_construct_(uint32_t& acc_count, tree_t<RT>* dest, range_of_element_type<maybe_tree_t<RT>> auto&& xs) -> void
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
		text_info_t acc;
		for (auto&& x : children())
		{
			acc = acc + x.info();
		}
		return acc;
	}

	template <typename RT>
	inline auto node_internal_t<RT>::child_at(size_type idx) const -> tree_t<RT> const&
	{
		ATMA_ASSERT(idx < size_);

		return children_[idx];
	}

	template <typename RT>
	inline auto node_internal_t<RT>::child_at(size_type idx) -> tree_t<RT>&
	{
		ATMA_ASSERT(idx < size_);

		return children_[idx];
	}

	template <typename RT>
	inline auto node_internal_t<RT>::try_child_at(uint32_t idx) -> maybe_tree_t<RT>
	{
		if (0 <= idx && idx < size_)
			return children_[idx];
		else
			return {};
	}

	template <typename RT>
	inline auto node_internal_t<RT>::append(tree_t<RT> const& info)
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

		return std::span<tree_t<RT> const>(children_.data(), limit);
	}

	template <typename RT>
	template <typename F>
	inline void node_internal_t<RT>::for_each_child(F f) const
	{
		for (auto& x : children_)
		{
			if (!x.node_pointer())
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
namespace atma::_rope_
{
	template <typename RT>
	inline node_t<RT>::node_t(node_type_t node_type, uint32_t height)
		: node_type_(node_type)
		, height_(height)
	{
	}

	template <typename RT>
	inline constexpr bool node_t<RT>::is_branch() const
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
		bool const is_not_mid_crlf = is_buffer_boundary || !is_seam(buf[byte_idx - 1], buf[byte_idx]);

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
		-> std::tuple<tree_t<RT>, tree_t<RT>>
	{
		ATMA_ASSERT(!insbuf.empty());
		//ATMA_ASSERT(byte_idx < hostbuf.size());
		if (byte_idx < hostbuf.size())
		{
			ATMA_ASSERT(utf8_byte_is_leading((byte const)hostbuf[byte_idx]));
		}

		size_t const insbuf_size = insbuf.size_bytes();

		// determine split point
		size_t redist_split_idx = 0;
		size_t insbuf_split_idx = 0;
		{
			// stack-allocate our splitbuf
			//
			// this is a very small (8-ish chars) buffer that we will use to determine
			// exactly upon which character to split our *resultant* buffer.
			// 
			// note that this split location is the *redistribution* split point, not
			// the insert point. the redistribution buffer is fictional - we are synthesizing
			// what it would look like if we zoomed in on a few chars located around the
			// middle of it - that's where we're going to split this fictional buffer.
			//
			// we are doing a bunch of confusing ops to ensure that regardless of where
			// the insbuf is/was inserted, we correctly redistribute the characters of
			// the entire fictional redistribute buffer across two halves.
			//
			// we're going to great lengths to remove any extraneous allocations

			constexpr size_t splitbuf_size{8};
			constexpr size_t splitbuf_halfsize{splitbuf_size / 2ull};
			static_assert(splitbuf_halfsize * 2 == splitbuf_size,
				"you gotta make your splitbuf size a multiple of two buddy");

			char splitbuf[splitbuf_size] ={0};


			// our fictional "redistribution buffer" is just what we'd have if we naively
			// inserted insbuf into hostbuf. we're prefixing anything in the redistribution-buffer
			// space with "redist". note there's not actual redistbuf
			size_t const redist_size = hostbuf.size() + insbuf.size();
			size_t const redist_midpoint = redist_size / 2;
			size_t const redist_halfsize = redist_size / 2;

			size_t const redist_ins_end_idx = byte_idx + insbuf.size();

			size_t const redist_splitbuf_copy_begin = redist_midpoint - std::min(splitbuf_halfsize, redist_halfsize);
			size_t const redist_splitbuf_copy_end = std::min(redist_size, redist_midpoint + splitbuf_halfsize);


			// fill our splitbuf from the characters around the midpoint of our redistbuf
			//
			// we'll do the following:
			//  1) fill from hostbuf up until we hit byte_idx, which is where insbuf
			//     would logically be after the insertion
			//  2) fill from insbuf until it's exhausted
			//  3) continue to fill from hostbuf after the byte_idx
			//
			for (size_t i = redist_splitbuf_copy_begin; i != redist_splitbuf_copy_end; ++i)
			{
				size_t const splitbuf_i = i - redist_splitbuf_copy_begin;

				splitbuf[splitbuf_i]
					= (i < byte_idx) ? hostbuf[i]
					: (i < redist_ins_end_idx) ? insbuf[i - byte_idx]
					: hostbuf[i - insbuf_size]
					;
			}

			static_assert(std::ranges::contiguous_range<decltype(splitbuf)>);

			// find the split index in redist-space
			redist_split_idx = redist_splitbuf_copy_begin + find_split_point(
				xfer_src(splitbuf),
				redist_midpoint - redist_splitbuf_copy_begin);

			// if the split-index (in redist space) would be within where insbuf would
			// have been inserted, we need to know. otherwise it's just the bounds
			insbuf_split_idx
				= (redist_ins_end_idx <= redist_split_idx) ? 0
				: (redist_split_idx <= byte_idx) ? insbuf.size()
				: redist_split_idx - byte_idx
				;

			ATMA_ASSERT(utf8_byte_is_leading((byte const)splitbuf[redist_split_idx]));
		}

		tree_t<RT> new_lhs, new_rhs;

		size_t const split_idx = redist_split_idx;

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
			tree_t<RT>{new_lhs},
			tree_t<RT>{new_rhs});
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
	auto is_saturated(tree_t<RT> const& info) -> bool
	{
		return info.is_leaf() || info.child_count() >= RT::branching_factor;
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
			if (char_idx < acc_chars + child.info().characters)
				break;

			acc_chars += child.info().characters - child.info().dropped_characters;
			++child_idx;
		}

		return std::make_tuple(child_idx, char_idx - acc_chars);
	}

	template <typename RT>
	inline auto find_for_char_idx_within(node_internal_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		size_t child_idx = 0;
		size_t acc_chars = 0;
		for (auto const& child : x.children())
		{
			if (char_idx <= acc_chars + child.info().characters)
				break;

			acc_chars += child.info().characters - child.info().dropped_characters;
			++child_idx;
		}

		return std::make_tuple(child_idx, char_idx - acc_chars);
	}

	template <typename F, typename RT>
	inline auto for_all_text(F f, tree_t<RT> const& tree) -> void
	{
		tree.node().visit(
			[f](node_internal_t<RT> const& x)
			{
				x.for_each_child(atma::curry(&for_all_text<F, RT>, f));
			},
			[&tree, f](node_leaf_t<RT> const& leaf)
			{
				auto bufview = std::string_view{leaf.buf.data() + tree.info().dropped_bytes, tree.info().bytes};
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
	inline auto replace_(tree_t<RT> const& dest, size_t idx, tree_t<RT> const& replacee) -> tree_branch_t<RT>
	{
		ATMA_ASSERT(idx < dest.children().size(), "replace_: index out of bounds");

		auto const& children = dest.children();

		auto result_node = make_internal_ptr<RT>(
			dest.height(),
			xfer_src(children).take(idx),
			replacee,
			xfer_src(children).skip(idx + 1));

		auto result = tree_branch_t<RT>{result_node};
		return result;
	}

	template <typename RT>
	inline auto append_(tree_branch_t<RT> const& dest, tree_t<RT> const& insertee) -> tree_t<RT>
	{
		ATMA_ASSERT(dest.children().size() < RT::branching_factor, "append_: insufficient space");

		auto const& children = dest.children();

		// if we are only aware of X nodes, and the underlying node has exactly X nodes, then 
		bool const underlying_node_has_identical_nodes = dest.node().children().size() == children.size();

		if (underlying_node_has_identical_nodes && dest.node().has_space_for_another_child())
		{
			auto r = const_cast<tree_branch_t<RT>&>(dest).append(insertee);
			return r;
		}
		else
		{
			auto result_node = make_internal_ptr<RT>(
				dest.height(),
				children,
				insertee);

			auto result = tree_t<RT>{
				dest.info() + insertee.info(),
				(uint32_t)children.size() + 1,
				result_node};

			return result;
		}
	}

	template <typename RT>
	inline auto insert_(tree_branch_t<RT> const& dest, size_t idx, tree_t<RT> const& insertee) -> treeop_result_t<RT>
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

			auto r = const_cast<tree_branch_t<RT>&>(dest)
				.append(insertee);

			return {r};
		}
		else if (space_for_additional_child && !insertion_is_at_back)
		{
			// the next-simplest case is when it's an insert in the middle. simply
			// recreate the node with the child in the middle.

			auto result_node = make_internal_ptr<RT>(
				dest.height(),
				xfer_src(children, idx),
				insertee,
				xfer_src_between(children, idx, children.size()));

			auto result = tree_branch_t<RT>{result_node};

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
					insertee,
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
					insertee,
					xfer_src(children).from(idx));
			}

			tree_branch_t<RT> lhs{ln};
			tree_branch_t<RT> rhs{rn};

			return treeop_result_t<RT>{lhs, rhs};
		}
	}

	template <typename RT>
	inline auto construct_from_(src_bounded_memxfer_t<tree_t<RT>> buf1, src_bounded_memxfer_t<tree_t<RT>> buf2, src_bounded_memxfer_t<tree_t<RT>> buf3) -> treeop_result_t<RT>
	{
		// this function takes three ranges of nodes and will redistribute
		size_t const buf1_size = buf1.size();
		size_t const buf2_size = buf2.size();
		size_t const buf3_size = buf3.size();

		auto full_size = buf1_size + buf2_size + buf3_size;

		if (full_size < RT::branching_factor)
		{
			auto result = make_internal_ptr<RT>(buf1, buf2, buf3);
			return result;
		}

		// split evenly
		auto split_idx = (full_size + 1ull) / 2;

		// case 1, split_idx is within buf1
		if (split_idx < buf1.size())
		{
			auto left = make_internal_ptr<RT>(
				buf1.to(split_idx));

			auto right = make_internal_ptr<RT>(
				buf1.from(split_idx),
				buf2,
				buf3);

			return {left, right};
		}
		// split_idx is within buf2
		else if (auto buf2_idx = split_idx - buf1_size; buf2_idx < buf2_size)
		{
			auto left = make_internal_ptr<RT>(
				buf1,
				buf2.to(buf2_idx));

			auto right = make_internal_ptr<RT>(
				buf2.from(buf2_idx),
				buf3);

			return {left, right};
		}
		else if (auto buf3_idx = split_idx - buf1_size - buf2_size; buf3_idx < buf3_size)
		{
			auto left = make_internal_ptr<RT>(
				buf1,
				buf2,
				buf3.to(buf3_idx));

			auto right = make_internal_ptr<RT>(
				buf3.from(buf3_idx));

			return {left, right};
		}

		return {};
	}





	//
	// a very common operation involves a child node needing to be replaced, with a possible
	// additional sibling-node requiring insertion.
	//
	template <typename RT>
	inline auto replace_and_insert_(tree_branch_t<RT> const& dest, size_t idx, tree_t<RT> const& repl_info, maybe_tree_t<RT> const& maybe_ins_info) -> treeop_result_t<RT>
	{
		ATMA_ASSERT(idx < dest.child_count(), "index for replacement out of bounds");

		auto& dest_node = dest.node();

		// in the simple case, this is just the replace operation
		if (!maybe_ins_info.has_value())
		{
			return {replace_(dest, idx, repl_info)};
		}

		auto& ins_info = maybe_ins_info.value();

		auto const children = dest_node.children();

		// we can fit the additional node in
		if (dest_node.has_space_for_another_child())
		{
			auto result_node = make_internal_ptr<RT>(
				dest_node.height(),
				xfer_src(children, idx),
				repl_info,
				ins_info,
				xfer_src(children, idx + 1, children.size() - idx - 1));

			auto result = tree_branch_t<RT>{result_node};

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

			return treeop_result_t<RT>{
				tree_branch_t<RT>{ln},
				tree_branch_t<RT>{rn}};
		}
	}

#define ATMA_ROPE_VERIFY_INFO_SYNC(info) \
		do { \
			if constexpr (ATMA_ENABLE_ASSERTS) \
			{ \
				tree_t<RT> _tmp_{(info).node}; \
				ATMA_ASSERT(_tmp_ == (info)); \
			} \
		} \
		while (0)

	template <typename RT>
	inline auto tree_merge_nodes_(tree_t<RT> const& left, tree_t<RT> const& right) -> tree_t<RT>
	{
		// validate assumptions:
		//  - both nodes are leaves, or both are internal
		//  - both nodes are at the same height
		ATMA_ASSERT(left.is_leaf() == right.is_leaf());
		ATMA_ASSERT(left.height() == right.height());
		auto const height = left.height();

		if constexpr (ATMA_ENABLE_ASSERTS)
		{
			tree_t<RT> right_node_info_temp{right.node_pointer()};
			ATMA_ASSERT(right_node_info_temp.info() == right.info());
		}

		auto result_text_info = left.info() + right.info();

		if (left.is_leaf())
		{
			// we know the height is two because we've just added an internal
			// node above our two leaf nodes

			auto result_node = make_internal_ptr<RT>(2u, left, right);
			return tree_t<RT>{result_text_info, result_node};
		}

		auto const& left_branch = left.as_branch();
		auto const& right_branch = right.as_branch();

		if (left.child_count() >= RT::minimum_branches && right.child_count() >= RT::minimum_branches)
		{
			// both nodes are saturated enough where we can just make a node above

			auto result_node = make_internal_ptr<RT>(height + 1, left, right);
			return tree_t<RT>{result_text_info, result_node};
		}
		else if (size_t const children_count = left.child_count() + right.child_count(); children_count <= RT::branching_factor)
		{
			// both nodes are so UNsaturated that we can merge the children into a new node

			auto result_node = make_internal_ptr<RT>(
				height,
				left_branch.children(),
				right_branch.children());

			return tree_t<RT>{result_text_info, result_node};
		}
		else
		{
			// nodes are too unbalanced. we need to redistribute. sum the nodes and
			// split in half, make sure left has majority in uneven cases

			size_t const new_right_children_count = children_count / 2;
			size_t const new_left_children_count = children_count - new_right_children_count;

			size_t const take_from_left_for_left = std::min(new_left_children_count, left_branch.children().size());
			size_t const take_from_right_for_left = new_left_children_count - take_from_left_for_left;
			size_t const take_from_right_for_right = std::min(new_right_children_count, right_branch.children().size());
			size_t const take_from_left_for_right = new_right_children_count - take_from_right_for_right;

			auto new_left_node = make_internal_ptr<RT>(
				height,
				xfer_src(left_branch.children(), take_from_left_for_left),
				xfer_src(right_branch.children(), take_from_right_for_left));

			tree_t<RT> new_left_tree{new_left_node};

			auto new_right_node = make_internal_ptr<RT>(
				height,
				xfer_src(left_branch.children()).last(take_from_left_for_right),
				xfer_src(right_branch.children()).last(take_from_right_for_right));

			tree_t<RT> new_right_tree{new_right_node};

			auto result_node = make_internal_ptr<RT>(
				height + 1,
				new_left_tree,
				new_right_tree);


			if constexpr (ATMA_ENABLE_ASSERTS)
			{
				auto new_result_info = new_left_tree.info() + new_right_tree.info();
				ATMA_ASSERT(result_text_info == new_result_info);
			}

			return tree_t<RT>{result_text_info, result_node};
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
		else if (idx == branch.child_count() - 1)
		{
			// all children from first node
			return branch;
		}
		else if (idx == 1)
		{
			return {branch.children().front()};
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
		ATMA_ASSERT(idx < branch.child_count());

		if (idx == branch.child_count() - 1)
		{
			// no children I guess!
			return {make_leaf_ptr<RT>()};
		}
		else if (idx == branch.child_count() - 2)
		{
			return {branch.children().back()};
		}
		else
		{
			tree_t<RT> info = make_internal_ptr<RT>(branch.height(), xfer_src(branch.children()).from(idx + 1));
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
		if constexpr (debug_internal_validation_v<RT>)
		{
			ATMA_ASSERT(validate_rope_(left));
			ATMA_ASSERT(validate_rope_(right));
		}

		if (left.height() < right.height())
		{
			auto const& right_branch = right.as_branch();
			auto right_children = right.children(); //info().node->as_branch().children();

			bool const left_is_saturated = is_saturated(left);
			bool const left_is_one_level_below_right = left.height() + 1 == right.height();

			if (left_is_saturated && left_is_one_level_below_right && right_branch.node().has_space_for_another_child())
			{
				auto n = insert_(right_branch, 0, left);
				ATMA_ASSERT(!n.right);
				return {n.left};
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
					tree_t<RT> n2 = make_internal_ptr<RT>(
						right.height(),
						xfer_src(right_children).skip(1));

					return tree_merge_nodes_(subtree, n2);
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

			bool const right_is_saturated = is_saturated(right);
			bool const right_is_one_level_below_left = right.height() + 1 == left.height();

			if (right_is_saturated && right_is_one_level_below_left && left_branch.node().has_space_for_another_child())
			{
				auto n = append_(left.as_branch(), right);
				//ATMA_ROPE_VERIFY_INFO_SYNC(n);
				return n;
			}
			else if (right_is_saturated && right_is_one_level_below_left)
			{
				auto [ins_left, ins_right] = insert_(left_branch, left.child_count(), right);
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
						left.child_count() - 1,
						subtree);

					return left_prime;
				}
				else if (subtree.height() == left.height())
				{
					auto left_prime_node = make_internal_ptr<RT>(
						left.height(),
						xfer_src(left_children).drop(1));

					return tree_merge_nodes_(tree_t<RT>{left_prime_node}, subtree);
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
			auto result = tree_merge_nodes_(left, right);
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
		return tree.node().visit(
			[&](node_internal_t<RT> const& x)
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
			[&](node_leaf_t<RT> const& x)
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
			[](tree_branch_t<RT> const& branch, size_t) { return std::make_tuple(branch.child_count() - 1); },
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
	inline auto edit_chunk_at_char(tree_t<RT> const& tree, size_t char_idx, F&& payload_function) -> edit_result_t<RT>
	{
		return navigate_to_leaf(tree, char_idx,
			tree_find_for_char_idx_within<RT>,
			std::forward<F>(payload_function),
			stitch_upwards_<RT>);
	}
}


namespace atma::_rope_
{
	template <typename RT>
	inline auto stitch_upwards_simple_(tree_branch_t<RT> const& branch, size_t child_idx, maybe_tree_t<RT> const& child_info) -> maybe_tree_t<RT>
	{
		return (child_info)
			? maybe_tree_t<RT>{ replace_(branch, child_idx, *child_info) }
			: maybe_tree_t<RT>{};
	}

	template <typename RT>
	inline auto stitch_upwards_(tree_branch_t<RT> const& branch, size_t child_idx, edit_result_t<RT> const& er) -> edit_result_t<RT>
	{
		auto const& [left_info, maybe_right_info, seam] = er;

		// validate that there's no seam between our two edit-result nodes. this
		// should have been caught in whatever edit operation was performed. stitching
		// the tree and melding crlf pairs is for siblings
		if (maybe_right_info && left_info.is_leaf())
		{
			auto const& leftleaf = left_info.as_leaf().node();
			auto const& rightleaf = maybe_right_info->as_leaf().node();

			ATMA_ASSERT(!is_seam(leftleaf.buf.back(), rightleaf.buf[maybe_right_info->info().dropped_bytes]));
		}

		// case 1: there's no seam. regardless if there's one or two children returned,
		//         we just call replace_and_insert_ and that's that.
		if (seam == seam_t::none)
		{
			auto r = replace_and_insert_(branch, child_idx, left_info, maybe_right_info);
			return {r.left, r.right, seam_t::none};
		}
		// case 2: there's a seam and we only have one node
		else if (seam != seam_t::none && !maybe_right_info)
		{
			// case 2.1: there's only a left-seam
			if (seam == seam_t::left)
			{
				// case 2.1.1 - we're at the front, can't do anything about it at this level
				if (child_idx == 0)
				{
					auto r = replace_<RT>(branch.as_tree(), child_idx, tree_t<RT>{left_info});
					return {r, {}, seam_t::left};
				}
				// case 2.1.2 - in the middle, try and remove seam
				else if (auto maybe_nodes = mend_left_seam_(seam, branch.child_at((int)child_idx - 1), left_info))
				{
#if 0
					auto const& [prev, child] = *maybe_nodes;
					auto [rl, rr] = construct_from_(
						xfer_src(branch.children()).to(child_idx - 1),
						xfer_src_list({prev, child}),
						xfer_src(branch.children()).from(child_idx + 1));
					
					ATMA_ASSERT(!rr);
					return {rl, rr, seam_t::none};
#endif
					return {};
				}
				// case 2.1.3 - we could not mend the seam (there was no corresponding cr)
				else
				{
					auto r = replace_(branch, child_idx, left_info);
					return {r, {}, seam_t::none};
				}
			}
			// case 2.2: there's only a right-seam
			else if (seam == seam_t::right)
			{
				// case 2.2.1 - we're at the back, nothing to do
				if (child_idx == branch.children().size() - 1)
				{
					auto r = replace_<RT>(branch, child_idx, tree_t<RT>{left_info});
					return {r, {}, seam_t::right};
				}
				// case 2.2.2 - in the middle, try and remove seam
				else if (auto maybe_nodes = mend_right_seam_(seam, left_info, branch.child_at((int)child_idx + 1)))
				{
					auto const& [child, next] = *maybe_nodes;
					auto [rl, rr] = replace_and_insert_<RT>(branch, child_idx, child, next);
					ATMA_ASSERT(!rr);
					return {rl, rr, seam_t::none};
				}
				// case 2.2.3 - we could not mend the seam
				else
				{
					auto r = replace_(branch, child_idx, left_info);
					return {r, {}, seam_t::none};
				}
			}
			// case 2.3: there's both seams
			else
			{
				// oh jesus
			}
		}

#if 0
		seam_t const left_seam = (seam & seam_t::left);
		seam_t const right_seam = (seam & seam_t::right);

		// the two (might be one!) nodes of our edit-result are sitting in the middle
		// two positions of these children. the potentially-stitched siblings will lie
		// on either side.
		auto mended_children = std::array<maybe_tree_t<RT>, 4>
			{
				node.try_child_at((int)child_idx - 1),
				left_info,
				maybe_right_info,
				node.try_child_at((int)child_idx + 1)
			};

		// left seam
		if (left_seam != seam_t::none)
		{
			auto const [mended_left_seam, maybe_mended_left_nodes] = mend_left_seam_(left_seam, mended_children[0], *mended_children[1]);
			if (maybe_mended_left_nodes)
			{
				mended_children[0] = std::get<0>(*maybe_mended_left_nodes);
				mended_children[1] = std::get<1>(*maybe_mended_left_nodes);
			}
		}

		// right seam
		if (right_seam != seam_t::none)
		{
			auto& right_seam_node = mended_children[2] ? *mended_children[2] : *mended_children[1];
			auto const [mended_right_seam, maybe_mended_right_nodes] = mend_right_seam_(right_seam, right_seam_node, mended_children[3]);
			if (maybe_mended_right_nodes)
			{
				right_seam_node = std::get<0>(*maybe_mended_right_nodes);
				mended_children[3] = std::get<1>(*maybe_mended_right_nodes);
			}
		}


		// if the edit_result we're stitching had two parts, and we were full,
		// then we must split the node. this is the same as replace_and_insert_
		// splitting us
		if (mended_children[2] && branch.children().size() == RT::branching_factor)
		{
			auto const left_size = RT::branching_factor / 2 + 1;
			auto const right_size = RT::branching_factor / 2;

			// case 1: split is to left of both nodes   =>  N N N L R
			if (child_idx < left_size)
			{
				//auto result_left = make_internal_ptr<RT>(
				//	branch.height(),
				//	
				//	xfer_src(mended_children, 4),
				//	xfer_src(branch.children()).from(std::min(child_idx + 2, branch.children().size())));
			}
		}

		auto blah = make_internal_ptr<RT>(
			branch.height(),
			xfer_src(branch.children()).take(std::max((int)child_idx - 2, 0)),
			xfer_src(mended_children, 4),
			xfer_src(branch.children()).from(std::min(child_idx + 2, branch.children().size())));
#endif

		return {};
	}

	template <typename RT>
	inline auto mend_left_seam_(seam_t seam, tree_t<RT> const& sibling_node, tree_t<RT> const& seam_node) -> mend_result_type<RT>
	{
		// append lf to the back of the previous leaf. if this returns an empty optional,
		// then that leaf didn't have a trailing cr at the end, and we are just going to
		// leave this lf as it is... sitting all pretty at the front

		auto sibling_node_prime = navigate_to_back_leaf(
			tree_t<RT>{sibling_node},
			append_lf_<RT>,
			stitch_upwards_simple_<RT>);

		if (sibling_node_prime)
		{
			// drop lf from seam node
			auto seam_node_prime = navigate_to_front_leaf(tree_t<RT>{seam_node},
				drop_lf_<RT>,
				stitch_upwards_simple_<RT>);

			ATMA_ASSERT(seam_node_prime);

			return {std::make_tuple(*sibling_node_prime, *seam_node_prime)};
		}

		return {};
	}

	template <typename RT>
	inline auto mend_right_seam_(seam_t seam, tree_t<RT> const& seam_node, tree_t<RT> const& sibling_node) -> mend_result_type<RT>
	{
		// drop lf from the front of the next leaf. if this returns an empty optional,
		// then that leaf didn't have an lf at the front, and we are just going to
		// leave this cr as is... just, dangling there

		auto sibling_node_prime = navigate_to_front_leaf(
			tree_t<RT>{sibling_node},
			drop_lf_<RT>,
			stitch_upwards_simple_<RT>);

		if (sibling_node_prime)
		{
			// append lf to seamed leaf
			auto seam_node_prime = navigate_to_back_leaf(tree_t<RT>{seam_node},
				append_lf_<RT>,
				stitch_upwards_simple_<RT>);

			ATMA_ASSERT(seam_node_prime);

			return {std::make_tuple(*seam_node_prime, *sibling_node_prime)};
		}

		return {};
	}
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto insert(size_t char_idx, tree_t<RT> const& dest, src_buf_t const& insbuf) -> edit_result_t<RT>
	{
		return edit_chunk_at_char(tree_t<RT>{dest}, char_idx,
			atma::bind(insert_small_text_<RT>, arg1, arg2, insbuf));
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

			return {1, {}, leaf};
		}
		else if (split_idx == leaf.info().bytes)
		{
			// in this case, we are asking to split at the end of our buffer.
			// let's instead return a no-result for the rhs, and pass this
			// node on the lhs.

			return {1, leaf, {}};
		}
		else
		{
			// okay, our char-idx has landed us in the middle of our buffer.
			// split the buffer into two and return both nodes.

			auto left = tree_leaf_t<RT>{make_leaf_ptr<RT>(leaf.data().to(split_idx))};
			auto right = tree_leaf_t<RT>{make_leaf_ptr<RT>(leaf.data().from(split_idx))};

			return {1, left, right};
		}
	}

	template <typename RT>
	inline auto split_up_fn_(tree_branch_t<RT> const& dest, size_t split_idx, split_result_t<RT> const& result) -> split_result_t<RT>
	{
		auto const& [orig_height, split_left, split_right] = result;

		// "we", in this stack-frame, are one level higher than what was returned in result
		uint32_t const our_height = orig_height + 1;

		// validate assumptions:
		//
		//  - split_idx is valid
		//  - either split_left, split_right, or both are valid
		ATMA_ASSERT(split_idx < dest.children().size());
		ATMA_ASSERT(!!split_left || !!split_right);
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
		if constexpr (debug_internal_validation_v<RT>)
		{
			ATMA_ASSERT(validate_rope_.internal_node(dest.as_tree()));

			if (split_left.has_value() && split_left.value().is_branch())
			{
				auto const& left_branch = split_left.value().as_branch();
				for (auto const& x : left_branch.children())
				{
					ATMA_ASSERT(validate_rope_.internal_node(x));
				}
			}

			if (split_right.has_value() && split_right.value().is_branch())
			{
				auto const& right_branch = split_right.value().as_branch();
				for (auto const& x : right_branch.children())
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
			if (!split_right)
			{
				auto right = make_internal_ptr<RT>(our_height, dest.children().subspan(1));
				return {our_height, split_left, right};
			}
			else if (split_right.value().is_saturated() && split_right.value().height() == orig_height)
			{
				// optimization: if split_right is a valid child-node, just change idx 0 to split_right
				auto right = replace_(dest, split_idx, split_right.value());

				return {our_height, split_left, right};
			}
			else
			{
				auto rhs_children = node_split_across_rhs_(dest, split_idx);
				auto right = tree_concat_(split_right.value(), rhs_children);

				return {our_height, split_left, right};
			}
		}
		else if (split_idx == dest.child_count() - 1)
		{
			if (!split_left)
			{
				auto left = make_internal_ptr<RT>(our_height, dest.children().subspan(0, split_idx));
				return {our_height, left, split_right};
			}
			else if (split_left.value().is_saturated() && split_left.value().height() == orig_height)
			{
				// optimization: if split_left is a valid child-node, just replace
				// the last node of our children to split_left
				auto left = replace_(dest, split_idx, split_left.value());
				return {our_height, left, split_right};
			}
			else
			{
				auto lhs_children = node_split_across_lhs_(dest, split_idx);
				auto left = tree_concat_(lhs_children, split_left.value());
				return {our_height, left, split_right};
			}
		}
		else
		{
			auto [our_lhs_children, our_rhs_children] = node_split_across_<RT>(
				dest,
				split_idx);

			auto left = (!split_left.has_value())
				? our_lhs_children
				: tree_concat_<RT>(our_lhs_children, split_left.value());

			auto right = (!split_right.has_value())
				? our_rhs_children
				: tree_concat_<RT>(split_right.value(), our_rhs_children);

			return {our_height, left, right};
		}
	}

	// returns: <index-of-child-node, remaining-characters>
	template <typename RT>
	inline auto tree_find_for_char_idx(tree_branch_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		return find_for_char_idx(x.node(), char_idx);
	}

	template <typename RT>
	inline auto tree_find_for_char_idx_within(tree_branch_t<RT> const& x, size_t char_idx) -> std::tuple<size_t, size_t>
	{
		return find_for_char_idx_within(x.node(), char_idx);
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

namespace atma::_rope_
{
	template <typename RT>
	auto erase(tree_t<RT> const& tree, size_t char_idx, size_t size_in_chars) -> erase_result_t<RT>
	{
		return erase_(tree, char_idx, char_idx + size_in_chars);
	}

	template <typename RT>
	auto erase_(tree_t<RT> const& tree, size_t rel_char_idx, size_t rel_char_end_idx) -> erase_result_t<RT>
	{
		// branches need to navigate to the correct children
		if (tree.node().is_branch())
		{
			auto const& branch = tree.as_branch();

			ATMA_ASSERT(rel_char_idx < rel_char_end_idx);
			ATMA_ASSERT(rel_char_idx <= branch.info().characters);
			ATMA_ASSERT(rel_char_end_idx <= branch.info().characters);

			// find begin & end children
			auto [start_child_idx, start_char_idx] = find_for_char_idx(branch.node(), rel_char_idx);
			auto [end_child_idx, end_char_idx] = find_for_char_idx_within(branch.node(), rel_char_end_idx);


			// case 1: the section to erase is entirely within one child
			if (start_child_idx == end_child_idx)
			{
				auto [result_lhs, result_rhs] = erase_(tree_t<RT>{
					branch.children()[start_child_idx]},
					start_char_idx, end_char_idx);

				if (result_lhs)
				{
					auto [ins_lhs, ins_rhs] = replace_and_insert_(branch, start_child_idx, *result_lhs, result_rhs);
					return {ins_lhs, ins_rhs};
				}
				else if (result_rhs)
				{
					auto result = replace_(branch, start_child_idx, *result_rhs);
					return {result};
				}
				else
				{
					// we lost the whole node, a.k.a. the char-idxs happened to be the
					// first and last indexes of this buffer

					auto new_branch = make_internal_ptr<RT>(branch.height(),
						xfer_src(branch.children()).to(start_child_idx),
						xfer_src(branch.children()).from(start_child_idx + 1));

					return {new_branch};
				}
			}
			// case 2: what to delete spans more than one child
			else
			{
				auto const& start_child = tree_t<RT>{branch.children()[start_child_idx]};
				auto const& end_child = tree_t<RT>{tree.children()[end_child_idx]};

				// what we need to do is the following:
				//   1) recurse into the two children in question
				//   2) delete all intervening children
				//   3) recreate the node with the remnants of the two nodes
				//
				// note: because the range of characters to delete is crossing the boundary
				//       of child nodes, we can guarantee that neither result of our recursion
				//       into our two children endpoints will return two parts, as the
				//       range of characters extends fully past one of their ends. they *may*
				//       return zero parts, if the full buffer was erased.
				//
				//       this means we can never have more nodes than previously,
				//       which simplifies our upwards stitching.


				// erase at the start of the range
				auto [start_lhs, start_rhs] = erase_(
					start_child,
					start_char_idx, start_child.info().characters);

				// erase at the end
				auto [end_lhs, end_rhs] = erase_(
					end_child,
					0u, end_char_idx);
				
				auto new_branch = make_internal_ptr<RT>(branch.height(),
					xfer_src(branch.children()).to(start_child_idx),
					start_lhs, start_rhs,
					end_lhs, end_rhs,
					xfer_src(branch.children()).from(end_child_idx + 1));

				return {new_branch};
			}
		}
		// we're a leaf, we need to split our buffer
		else
		{
			auto const& leaf = tree.as_leaf();

			// case 1: the whole node is deleted
			if (rel_char_idx == 0 && rel_char_end_idx == tree.info().characters)
			{
				return {};
			}
			// case 2: there's leftover bits on the lhs
			//   - split buffer & return lhs
			else if (rel_char_idx > 0 && rel_char_end_idx == tree.info().characters)
			{
				auto byte_idx = leaf.byte_idx_from_char_idx(rel_char_idx);
				auto lhs = make_leaf_ptr<RT>(leaf.data().take(byte_idx));
				return {lhs, {}};
			}
			// case 3: there's leftover bits on the rhs
			//  - split buffer & return rhs
			else if (rel_char_idx == 0 && rel_char_end_idx < tree.info().characters)
			{
				auto byte_idx = leaf.byte_idx_from_char_idx(rel_char_end_idx);
				auto rhs = make_leaf_ptr<RT>(leaf.data().from(byte_idx));
				return {{}, rhs};
			}
			// case 4: this delete operation is fully within this buffer
			//  - recreate the buffer without the missing bit and return the node
			else
			{
				auto start_byte_idx = leaf.byte_idx_from_char_idx(rel_char_idx);
				auto end_byte_idx = leaf.byte_idx_from_char_idx(rel_char_end_idx);

				auto new_leaf = make_leaf_ptr<RT>(
					leaf.data().take(start_byte_idx),
					leaf.data().from(end_byte_idx));

				return {new_leaf};
			}
		}
	}
}



namespace atma::_rope_
{
	template <typename RT>
	inline auto insert_small_text_(tree_leaf_t<RT> const& leaf, size_t char_idx, src_buf_t insbuf) -> edit_result_t<RT>
	{
		ATMA_ASSERT(insbuf.size() <= RT::buf_edit_max_size);
		auto const& leaf_node = leaf.node();
		auto const& leaf_info = leaf.info();

		// early out
		if (insbuf.empty())
		{
			return {leaf};
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
		bool const inserting_at_end = char_idx == leaf.info().characters;
		bool const cr_at_end = insbuf[insbuf.size() - 1] == charcodes::cr;
		seam_t const right_seam = (inserting_at_end && cr_at_end) ? seam_t::right : seam_t::none;


		// calculate byte index from character index
		auto byte_idx = utf8_charseq_idx_to_byte_idx(leaf.data().data(), leaf.data().size(), char_idx);

		// if we want to append, we must first make sure that the (immutable, remember) buffer
		// does not have any trailing information used by other trees. secondly, we must make
		// sure that the byte_idx of the character is actually the last byte_idx of the buffer
		bool buf_is_appendable = (leaf_info.dropped_bytes + leaf_info.bytes) == leaf_node.buf.size();
		bool byte_idx_is_at_end = leaf_info.bytes == byte_idx;
		bool can_fit_in_chunk = leaf_info.bytes + insbuf.size() <= RT::buf_edit_max_size;

		ATMA_ASSERT(inserting_at_end == byte_idx_is_at_end);

		// simple append is possible
		if (can_fit_in_chunk && inserting_at_end && buf_is_appendable)
		{
			auto& mut_buf = const_cast<typename node_leaf_t<RT>::buf_t&>(leaf_node.buf);

			mut_buf.append(insbuf);

			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;
			auto result = tree_leaf_t<RT>{result_info, 0, leaf.node_pointer()};

			return _rope_::edit_result_t<RT>{result, {}, left_seam | right_seam};
		}
		// we're not appending, but we can fit within the chunk: reallocate & insert
		else if (can_fit_in_chunk)
		{
			auto affix_string_info = _rope_::text_info_t::from_str(insbuf.data(), insbuf.size());
			auto result_info = leaf_info + affix_string_info;

			auto result_node = _rope_::make_leaf_ptr<RT>(
					leaf.data().take(byte_idx),
					insbuf,
					leaf.data().from(byte_idx));
			
			auto result = tree_leaf_t<RT>{result_info, 0, result_node};

			return _rope_::edit_result_t<RT>(result, {}, left_seam | right_seam);
		}
		// any other case: splitting is required
		else
		{
			auto [lhs_info, rhs_info] = _rope_::insert_and_redistribute<RT>(leaf_info, leaf.data(), insbuf, byte_idx);

			return _rope_::edit_result_t<RT>{lhs_info, rhs_info, left_seam | right_seam};
		}
	}
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto drop_lf_(tree_leaf_t<RT> const& leaf, size_t) -> maybe_tree_t<RT>
	{
		ATMA_ASSERT(!leaf.data().empty());

		if (leaf.data().front() == charcodes::lf)
		{
			ATMA_ASSERT(leaf.info().line_breaks > 0);

			auto result = leaf.info()
				+ text_info_t{.dropped_bytes = 1, .dropped_characters = 1}
				- text_info_t{.bytes = 1, .characters = 1, .line_breaks = 1};

			return tree_t<RT>{result, leaf.node_pointer()};
		}
		else
		{
			return {};
		}
	}

	template <typename RT>
	inline auto append_lf_(tree_leaf_t<RT> const& leaf, size_t) -> maybe_tree_t<RT>
	{
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

		const_cast<node_leaf_t<RT>&>(leaf.node())
			.buf.push_back(charcodes::lf);

		auto result_info = leaf.info() + text_info_t{.bytes = 1, .characters = 1};
		auto result = tree_t<RT>{result_info, leaf.node_pointer()};
		return result;
	}
}














































































namespace atma::_rope_
{
	template <typename RT>
	inline auto validate_rope_t_::check_node(tree_t<RT> const& tree, size_t min_children) -> check_node_result_type
	{
		return tree.node().visit(
			[&tree, min_children](node_internal_t<RT> const& internal_node) -> check_node_result_type
			{
				if (tree.child_count() < min_children)
					return {false, 0};

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
	inline auto validate_rope_t_::operator ()(tree_t<RT> const& x) const -> bool
	{
		// assume we're passed the root (hence '2' as the minimum branch)
		return std::get<0>(check_node(x, 2));
	}

	template <typename RT>
	inline auto validate_rope_t_::internal_node(tree_t<RT> const& x) const -> bool
	{
		return std::get<0>(check_node(x, RT::minimum_branches));
	}
}








namespace atma::_rope_
{
	template <typename RT>
	struct build_rope_t_
	{
		inline static constexpr size_t max_level_size = RT::branching_factor + ceil_div<size_t>(RT::branching_factor, 2u);

		using level_t = std::tuple<uint32_t, std::array<tree_t<RT>, max_level_size>>;
		using level_stack_t = std::list<level_t>;

		auto stack_level(level_stack_t& stack, uint32_t level) const -> level_t&;
		decltype(auto) stack_level_size(level_stack_t& stack, uint32_t level) const { return std::get<0>(stack_level(stack, level)); }
		decltype(auto) stack_level_array(level_stack_t& stack, uint32_t level) const { return std::get<1>(stack_level(stack, level)); }
		decltype(auto) stack_level_size(level_t const& x) const { return std::get<0>(x); }
		decltype(auto) stack_level_array(level_t const& x) const { return std::get<1>(x); }

		void stack_push(level_stack_t& stack, uint32_t level, tree_t<RT> const& insertee) const;
		auto stack_finish(level_stack_t& stack) const -> tree_t<RT>;

		auto operator ()(src_buf_t str) const -> tree_t<RT>;
	};

	template <typename RT>
	constexpr build_rope_t_<RT> build_rope_;
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto build_rope_t_<RT>::stack_level(level_stack_t& stack, uint32_t level) const -> level_t&
	{
		auto i = stack.begin();
		std::advance(i, level);
		return *i;
	}

	template <typename RT>
	inline void build_rope_t_<RT>::stack_push(level_stack_t& stack, uint32_t level, tree_t<RT> const& insertee) const
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
			tree_t<RT> higher_level_node{make_internal_ptr<RT>(
				uint32_t(level + 2),
				xfer_src(array.data(), RT::branching_factor))};

			// erase those node-infos and move the trailing ones to the front
			// don't forget to reset the size
			{
				auto const remainder_size = max_level_size - RT::branching_factor;

				atma::memory_destruct(
					xfer_dest(array).to(RT::branching_factor));
				
				atma::memory_relocate(
					xfer_dest(array, remainder_size),
					xfer_src(array).from(RT::branching_factor));
				
				atma::memory_default_construct(
					xfer_dest(array).from(remainder_size));

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
	inline auto build_rope_t_<RT>::stack_finish(level_stack_t& stack) const -> tree_t<RT>
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

				tree_t<RT> higher_level_node{make_internal_ptr<RT>(
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

				tree_t<RT> higher_level_lhs_node{make_internal_ptr<RT>(
					uint32_t(level + 2),
					xfer_src(level_array.data(), lhs_size))};

				tree_t<RT> higher_level_rhs_node{make_internal_ptr<RT>(
					uint32_t(level + 2),
					xfer_src(level_array.data() + lhs_size, rhs_size))};

				stack_push(stack, level + 1, higher_level_lhs_node);
				stack_push(stack, level + 1, higher_level_rhs_node);
			}
		}

		return tree_t<RT>{};
	}

	template <typename RT>
	inline auto build_rope_t_<RT>::operator ()(src_buf_t str) const -> tree_t<RT>
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

			auto new_leaf = tree_t<RT>{make_leaf_ptr<RT>(leaf_text)};

			stack_push(stack, 0, new_leaf);
		}

		// stack fixup
		auto r = stack_finish(stack);
		return r;
	}
}

























































namespace atma::_rope_
{
	class append_node_t_
	{
		template <typename RT>
		auto append_node_(tree_t<RT> const& dest, node_ptr<RT> const& ins) const -> treeop_result_t<RT>;

	public:
		template <typename RT>
		auto operator ()(tree_t<RT> const& dest, node_ptr<RT> x) const -> tree_t<RT>;
	};

	constexpr class append_node_t_ append_node_;
}

namespace atma::_rope_
{
	template <typename RT>
	inline auto append_node_t_::append_node_(tree_t<RT> const& dest, node_ptr<RT> const& ins) const -> treeop_result_t<RT>
	{
		return dest.node().visit(
			[&](node_internal_t<RT> const& x)
			{
				auto const children = x.children();
				ATMA_ASSERT(children.size() >= RT::branching_factor / 2);

				// recurse if our right-most child is an internal node
				if (auto const& last = children[children.size() - 1]; last.is_branch())
				{
					auto [left, maybe_right] = append_node_(last, ins);
					auto right = maybe_branch_to_maybe_tree(maybe_right); //maybe_right.has_value() ? maybe_tree_t<RT>{maybe_right.value().as_tree()} : maybe_tree_t<RT>{std::nullopt};
					return replace_and_insert_(dest.as_branch(), children.size() - 1, left, right);
				}
				// it's a leaf, so insert @ins into us
				else
				{
					return insert_(dest.as_branch(), children.size(), tree_t<RT>{ins});
				}
			},
			[&](node_leaf_t<RT> const& x)
			{
				// the only way we can be here is if the root was a leaf node we must root-split.
				auto r = make_internal_ptr<RT>(dest.height() + 1, dest, tree_t<RT>{ins});
				return treeop_result_t<RT>{r};
			});
	}

	template <typename RT>
	auto append_node_t_::operator ()(tree_t<RT> const& dest, node_ptr<RT> x) const -> tree_t<RT>
	{
		tree_t<RT> result;

		if (dest.node_pointer() == node_ptr<RT>::null)
		{
			return tree_t<RT>{x};
		}
		else
		{
			auto const [lhs, rhs] = append_node_(dest, x);

			if (rhs.has_value())
			{
				return tree_t<RT>{make_internal_ptr<RT>(dest.height() + 1, lhs, *rhs)};
			}
			else
			{
				return lhs;
			}
		}
	};




	template <typename RT>
	inline auto build_rope_naive(src_buf_t str) -> tree_t<RT>
	{
		// remove null terminator if necessary
		if (str[str.size() - 1] == '\0')
			str = str.take(str.size() - 1);

		tree_t<RT> root;

		// case 1. the str is small enough to just be inserted as a leaf
		while (!str.empty())
		{
			size_t candidate_split_idx = std::min(str.size(), RT::buf_size);
			auto split_idx = find_split_point(str, candidate_split_idx, split_bias::hard_left);

			auto leaf_text = str.to(split_idx);
			str = str.skip(split_idx);

			if (root.node_pointer()  == node_ptr<RT>::null)
			{
				root = tree_t{make_leaf_ptr<RT>(leaf_text)};
			}
			else
			{
				root = append_node_(root, make_leaf_ptr<RT>(leaf_text));
			}
		}

		return root;
	}

}












































namespace atma
{
	template <typename RT>
	inline basic_rope_t<RT>::basic_rope_t()
		: root_(_rope_::make_leaf_ptr<RT>())
	{}

	template <typename RT>
	inline basic_rope_t<RT>::basic_rope_t(char const* string, size_t size)
		: root_{_rope_::build_rope_<RT>(xfer_src(string, size))}
	{}

	template <typename RT>
	inline basic_rope_t<RT>::basic_rope_t(_rope_::tree_t<RT> const& root_info)
		: root_(root_info)
	{}

	template <typename RT>
	inline auto basic_rope_t<RT>::push_back(char const* str, size_t sz) -> void
	{
		this->insert(root_.info().characters, str, sz);
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
#if 0
		else if (sz <= RT::buf_edit_max_size * 6)
		{
			// chunkify
			for (size_t i = 0; i != sz; )
			{
				[[maybe_unused]] size_t chunk_size = std::min((sz - i), RT::buf_edit_max_size);

			}
		}
#endif
		else
		{
			// this is a "big" string, so:
			//  a) build a rope from the big string
			//  b) split us (the tree) in twain
			//  c) concatenate the ropes together
			//
			// optimizations provided for when we're inserting at the front or at the end
			
			auto ins_tree = _rope_::build_rope_<RT>(xfer_src(str, sz));

			if (char_idx == 0)
			{
				//root_
				auto r = _rope_::tree_concat_<RT>(
					ins_tree,
					root_);

				if constexpr (_rope_::debug_internal_validation_v<RT>)
				{
					ATMA_ASSERT(atma::_rope_::validate_rope_(r));
				}

				edit_result.left = r;
			}
			else if (char_idx == root_.info().characters)
			{
				auto r = _rope_::tree_concat_<RT>(
					root_,
					ins_tree);

				edit_result.left = r;
			}
			else
			{
				auto [depth, left, right] = _rope_::split(_rope_::tree_t{root_}, char_idx);

				auto r1 = left.has_value()
					? _rope_::tree_concat_<RT>(left.value(), ins_tree)
					: ins_tree;

				auto r2 = right.has_value()
					? _rope_::tree_concat_<RT>(r1, right.value())
					: r1;

				edit_result.left = r2;
			}
		}

		if (edit_result.right.has_value())
		{
			auto info = edit_result.left.info() + (* edit_result.right).info();

			auto node = _rope_::make_internal_ptr<RT>(
				edit_result.left.node().height() + 1,
				edit_result.left,
				*edit_result.right);

			root_ = _rope_::tree_t<RT>{info, 2, node};
		}
		else
		{
			root_ = edit_result.left;
		}
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::erase(size_t char_idx, size_t size_in_chars) -> void
	{
		// the easiest implementation is to split the tree twice, once at each character location
		//
		// but is this the best implementation?
		auto [left, right] = _rope_::erase(_rope_::tree_t<RT>{root_}, char_idx, size_in_chars);

		ATMA_ASSERT(left, "erase_result_t must return at least the lhs");

		if (right.has_value())
		{
			auto info = left.value().info() + right.value().info();

			auto node = _rope_::make_internal_ptr<RT>(
				left.value().node().height() + 1,
				left.value(),
				right.value());

			root_ = _rope_::tree_t<RT>{info, 2, node};
		}
		else
		{
			root_ = *left;
		}
	}

	template <typename RT>
	inline auto basic_rope_t<RT>::split(size_t char_idx) const -> std::tuple<basic_rope_t<RT>, basic_rope_t<RT>>
	{
		auto [depth, left, right] = _rope_::split<RT>(root_, char_idx);

		return {
			basic_rope_t{left.value_or(_rope_::tree_t<RT>{_rope_::make_leaf_ptr<RT>()})},
			basic_rope_t{right.value_or(_rope_::tree_t<RT>{_rope_::make_leaf_ptr<RT>()})}};
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

	template <typename RT>
	inline auto operator == (basic_rope_t<RT> const& lhs, basic_rope_t<RT> const& rhs) -> bool
	{
		if (static_cast<_rope_::tree_t<RT> const&>(lhs.root()).size_bytes() != static_cast<_rope_::tree_t<RT> const&>(rhs.root()).size_bytes())
			return false;

		_rope_::basic_leaf_iterator_t<RT> lhs_leaf_iter{ lhs };
		_rope_::basic_leaf_iterator_t<RT> rhs_leaf_iter{ rhs };
		_rope_::basic_leaf_iterator_t<RT> sentinel;

		// loop through leaves for both ropes
		size_t lhs_offset = 0;
		size_t rhs_offset = 0;
		while (lhs_leaf_iter != sentinel && rhs_leaf_iter != sentinel)
		{
			auto lhsd = lhs_leaf_iter->data().from(lhs_offset);
			auto rhsd = rhs_leaf_iter->data().from(rhs_offset);

			if (auto r = memory_compare(lhsd, rhsd); r == 0)
			{
				// memory contents compared completely, keep going
				++lhs_leaf_iter;
				++rhs_leaf_iter;
				lhs_offset = rhs_offset = 0;
			}
			else if (r == std::size(rhsd))
			{
				// memory contents compared equally, but rhs ran out of characters
				++rhs_leaf_iter;
				lhs_offset = r;
				rhs_offset = 0;
			}
			else if (-r == std::size(lhsd))
			{
				// memory contents compared equally, but lhs ran out of characters
				++lhs_leaf_iter;
				lhs_offset = 0;
				rhs_offset = -r;
			}
			else
			{
				// did not compare equally
				return false;
			}
		}

		// exhausted leaves to compare (all tested equal), so if we exhausted
		// both ropes equally, then they were completely equal
		return (lhs_leaf_iter == sentinel && rhs_leaf_iter == sentinel);
	}

	template <typename RT>
	inline auto operator == (basic_rope_t<RT> const& lhs, std::string_view rhs) -> bool
	{
		if (lhs.size() == 0)
			return rhs.empty();

		auto iter = basic_rope_char_iterator_t<RT>{ lhs };

		auto utf8range = utf8_const_range_t{ rhs.begin(), rhs.end() };
		for (auto&& x : utf8range)
		{
			if (*iter != x)
				return false;
			++iter;
		}

		return true;
	}

	template <typename RT, std::ranges::contiguous_range Range>
	requires std::same_as<std::remove_const_t<std::ranges::range_value_t<Range>>, char>
	auto operator == (basic_rope_t<RT> const& lhs, Range const& rhs) -> bool
	{
		if (lhs.size() == 0)
			return std::empty(rhs);

		auto iter = basic_rope_char_iterator_t<RT>{ lhs };

		auto utf8range = utf8_const_range_t{ std::ranges::begin(rhs), std::ranges::end(rhs) };
		for (auto&& x : utf8range)
		{
			if (*iter != x)
				return false;
			++iter;
		}

		return true;
	}

	using rope_t = basic_rope_t<rope_default_traits>;
}











namespace atma
{
	template <typename RT>
	inline basic_rope_char_iterator_t<RT>::basic_rope_char_iterator_t(basic_rope_t<RT> const& rope)
		: rope_(rope)
	{
		using namespace _rope_;

		std::tie(leaf_, leaf_characters_) = navigate_to_front_leaf<RT>(rope.root(), [](tree_leaf_t<RT> const& leaf, size_t)
			{
				return std::make_tuple(atma::intrusive_ptr_cast<node_leaf_t<RT>>(leaf.node_pointer()), leaf.info().characters);
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
				tree_find_for_char_idx<RT>,
				[](tree_leaf_t<RT> const& leaf, size_t)
				{
					return std::make_tuple(atma::intrusive_ptr_cast<node_leaf_t<RT>>(leaf.node_pointer()), leaf.info().characters);
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
		tree_stack_.push_back({rope.root().as_branch(), 0});
		while (std::get<0>(tree_stack_.back()).children()[0].is_branch())
		{
			tree_stack_.push_back({std::get<0>(tree_stack_.back()).children().front().as_branch(), 0});
		}
	}

	template <typename RT>
	inline auto basic_leaf_iterator_t<RT>::operator ++() -> basic_leaf_iterator_t<RT>&
	{
		auto* parent = &std::get<0>(tree_stack_.back());
		auto* idx = &std::get<1>(tree_stack_.back());

		++*idx;

		// walk up the stack
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

		ATMA_ASSERT(!tree_stack_.empty());

		auto get_deepest_child = [&]() -> tree_t<RT>
			{ return std::get<0>(tree_stack_.back()).child_at(std::get<1>(tree_stack_.back())); };

		// walk back down
		while (get_deepest_child().is_branch())
		{
			auto child = get_deepest_child();
			tree_stack_.push_back({child.as_branch(), 0});

			ATMA_ASSERT(!tree_stack_.empty() && std::get<0>(tree_stack_.back()).node_pointer());
		}

		return *this;
	}

	template <typename RT>
	inline auto basic_leaf_iterator_t<RT>::operator *() const -> tree_leaf_t<RT> const&
	{
		auto& [branch, idx] = tree_stack_.back();
		return static_cast<tree_leaf_t<RT> const&>(branch.child_at(idx));
	}

	template <typename RT>
	inline auto basic_leaf_iterator_t<RT>::operator ->() const -> tree_leaf_t<RT> const*
	{
		auto& [branch, idx] = tree_stack_.back();
		return static_cast<tree_leaf_t<RT> const*>(&branch.child_at(idx));
	}

	template <typename RT>
	inline auto operator == (basic_leaf_iterator_t<RT> const& lhs, basic_leaf_iterator_t<RT> const& rhs) -> bool
	{
		return std::ranges::equal(lhs.tree_stack_, rhs.tree_stack_);
	}
}





