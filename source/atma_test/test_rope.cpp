#include <atma/unit_test.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/assert.hpp>
#include <atma/rope.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>

#include <array>
#include <utility>
#include <iostream>
#include <concepts>

import atma.bind;
//import atma.rope;
import atma.memory;
import atma.intrusive_ptr;


namespace
{
	using test_rope_t = atma::basic_rope_t<atma::rope_test_traits>;

	char const* passage =
		"good evening, this is your captain speaking. \n"
		"unfortunately we forgot to fill up the plane \n"
		"before takeoff. sorry for the inconvenience, \n"
		"but I'm going to need some upstanding people \n"
		"to get out and push us to the closest petrol \n"
		"station. for your efforts you'll be rewarded \n"
		"with a $50 gift-coupon that is redeemable at \n"
		"any store within the food court.\n";

	size_t const passage_size = strlen(passage);
	size_t const passage_line_breaks = 8;

	auto const* insert_fragment =
		"\n"
		"haha just kidding. what I actually need is \n"
		"for everyone to get under the plane and blow \n"
		"upwards to keep us flying. ";
	auto const insert_fragment_size = strlen(insert_fragment);
}




SCENARIO("atma::rope's internal operations work")
{
	using T = atma::rope_test_traits;

	// just quickly grab the node for an insert/edit-result
	auto internal_node_of = [](auto&& x) -> decltype(auto) { return x.as_branch().node(); };
	auto children_of = [&](auto&& x) -> decltype(auto) { return internal_node_of(x).children(); };
	auto child_node_at = [&](auto&& x, size_t idx) -> decltype(auto) { return children_of(x)[idx].node_pointer(); };

	auto lhs_internal_node = [&](auto&& x) -> decltype(auto) { return internal_node_of(x.left); };
	auto lhs_children = [&](auto&& x) { return children_of(x.left); };
	auto lhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return child_node_at(x.left, idx); };

	auto rhs_internal_node = [&](auto&& x) -> decltype(auto) { return internal_node_of(x.right.value()); };
	auto rhs_children = [&](auto&& x) { return children_of(x.right.value()); };
	auto rhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return child_node_at(x.right.value(), idx); };


	GIVEN("several leaf nodes (\"A\", \"B\", \"C\"...) and corresponding node-infos")
	{
		auto A = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("A", 1));
		auto B = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("B", 1));
		auto C = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("C", 1));
		auto D = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("D", 1));
		auto X = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("X", 1));
		auto Y = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("Y", 1));

		atma::_rope_::tree_t<T> A_info{A};
		atma::_rope_::tree_t<T> B_info{B};
		atma::_rope_::tree_t<T> C_info{C};
		atma::_rope_::tree_t<T> D_info{D};
		atma::_rope_::tree_t<T> X_info{X};
		atma::_rope_::tree_t<T> Y_info{Y};

		// test insert where we have space
		AND_GIVEN("a default-created internal-node")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(1u);
			auto internal_info = atma::_rope_::tree_branch_t<T>{internal_node};

			WHEN("we _rope_::insert_ into our internal-node the node A at index 0")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postAresult.right.has_value());
				}

				THEN("the text-info is the sum information of A")
				{
					CHECK(postAresult.left.info().bytes == 1);
					CHECK(postAresult.left.info().characters == 1);
					CHECK(postAresult.left.child_count() == 1);
				}

				THEN("there is one child node that matches A")
				{
					CHECK(lhs_children(postAresult).size() == 1);
					CHECK(lhs_child_node(postAresult, 0) == A);
				}
			}

			WHEN("we _rope_::insert_ into our internal-node the node A at index 0, and node B at index 1")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);
				auto postBresult = atma::_rope_::insert_<T>(postAresult.left, 1, B_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postBresult.right.has_value());
				}

				THEN("the text-info is the sum information of A & B")
				{
					CHECK(postBresult.left.info().bytes == 2);
					CHECK(postBresult.left.info().characters == 2);
					CHECK(postBresult.left.child_count() == 2);
				}

				THEN("there are two nodes that match A & B")
				{
					CHECK(lhs_children(postBresult).size() == 2);
					CHECK(lhs_child_node(postBresult, 0) == A);
					CHECK(lhs_child_node(postBresult, 1) == B);
				}
			}

			WHEN("into internal-node we _rope_::insert_ nodes A, B, C, D at indices 0, 1, 2, 3 to saturate the node")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);
				auto postBresult = atma::_rope_::insert_<T>(postAresult.left, 1, B_info);
				auto postCresult = atma::_rope_::insert_<T>(postBresult.left, 2, C_info);
				auto postDresult = atma::_rope_::insert_<T>(postCresult.left, 3, D_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postDresult.right.has_value());
				}

				THEN("the text-info is the sum information of A & B")
				{
					CHECK(postDresult.left.info().bytes == 4);
					CHECK(postDresult.left.info().characters == 4);
					CHECK(postDresult.left.child_count() == 4);
				}

				THEN("there are four nodes that match A, B, C, D")
				{
					CHECK(lhs_child_node(postDresult, 0) == A);
					CHECK(lhs_child_node(postDresult, 1) == B);
					CHECK(lhs_child_node(postDresult, 2) == C);
					CHECK(lhs_child_node(postDresult, 3) == D);
				}
			}

			WHEN("into internal-node we _rope_::insert_ nodes A, B, C, D at index 0 each time")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);
				auto postBresult = atma::_rope_::insert_<T>(postAresult.left, 0, B_info);
				auto postCresult = atma::_rope_::insert_<T>(postBresult.left, 0, C_info);
				auto postDresult = atma::_rope_::insert_<T>(postCresult.left, 0, D_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postDresult.right.has_value());
				}

				THEN("the text-info is the sum information of A,B,C,D")
				{
					CHECK(postDresult.left.info().bytes == 4);
					CHECK(postDresult.left.info().characters == 4);
					CHECK(postDresult.left.child_count() == 4);
				}

				THEN("there are four nodes that match A, B, C, D, but in REVERSE ORDER")
				{
					CHECK(lhs_child_node(postDresult, 0) == D);
					CHECK(lhs_child_node(postDresult, 1) == C);
					CHECK(lhs_child_node(postDresult, 2) == B);
					CHECK(lhs_child_node(postDresult, 3) == A);
				}
			}
		}
	

		// test insert_ with splitting
		AND_GIVEN("a fully-saturated internal-node of [A, B, C, D]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(2u, A_info, B_info, C_info, D_info);
			auto internal_info = atma::_rope_::tree_branch_t<T>{internal_node};

			WHEN("we insert X at index 0")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 0, X_info);
			
				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains X, A, B")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == X);
					CHECK(lhs_child_node(postXresult, 1) == A);
					CHECK(lhs_child_node(postXresult, 2) == B);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.right.value().info().bytes == 2);
					CHECK(postXresult.right.value().info().characters == 2);
					CHECK(postXresult.right.value().child_count() == 2);
				}

				THEN("the rhs node contains C, D")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == C);
					CHECK(rhs_child_node(postXresult, 1) == D);
				}
			}


			WHEN("we insert X at index 1")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 1, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains A, X, B")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == X);
					CHECK(lhs_child_node(postXresult, 2) == B);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.right.value().info().bytes == 2);
					CHECK(postXresult.right.value().info().characters == 2);
					CHECK(postXresult.right.value().child_count() == 2);
				}

				THEN("the rhs node contains C, D")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == C);
					CHECK(rhs_child_node(postXresult, 1) == D);
				}
			}

			WHEN("we insert X at index 2")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 2, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains A, B, X")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.right.value().info().bytes == 2);
					CHECK(postXresult.right.value().info().characters == 2);
					CHECK(postXresult.right.value().child_count() == 2);
				}

				THEN("the rhs node contains C, D")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == C);
					CHECK(rhs_child_node(postXresult, 1) == D);
				}
			}
			
			WHEN("we insert X at index 3")
			{
				// 0 1 2 3
				// A B C D
				//       ^
				// 
				// A B C | X D
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 3, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains A, B, C")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == C);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.right.value().info().bytes == 2);
					CHECK(postXresult.right.value().info().characters == 2);
					CHECK(postXresult.right.value().child_count() == 2);
				}

				THEN("the rhs node contains X, D")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == X);
					CHECK(rhs_child_node(postXresult, 1) == D);
				}
			}

			WHEN("we insert X at index 4")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 4, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains A, B, C")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == C);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.right.value().info().bytes == 2);
					CHECK(postXresult.right.value().info().characters == 2);
					CHECK(postXresult.right.value().child_count() == 2);
				}

				THEN("the rhs node contains D, X")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == D);
					CHECK(rhs_child_node(postXresult, 1) == X);
				}
			}
		}

		// test replace_
		AND_GIVEN("a fully-saturated internal-node of [A, B, C, D]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(2u, A_info, B_info, C_info, D_info);
			auto internal_info = atma::_rope_::tree_branch_t<T>{internal_node};

			WHEN("we call replace_() at index 2 with node X")
			{
				auto postXresult = atma::_rope_::replace_<T>(internal_info, 2, X_info);

				THEN("the left node-info is four nodes worth")
				{
					CHECK(postXresult.info().bytes == 4);
					CHECK(postXresult.info().characters == 4);
					CHECK(postXresult.child_count() == 4);
				}

				THEN("the node now contains A, B, X, D")
				{
					CHECK(children_of(postXresult).size() == 4);
					CHECK(child_node_at(postXresult, 0) == A);
					CHECK(child_node_at(postXresult, 1) == B);
					CHECK(child_node_at(postXresult, 2) == X);
					CHECK(child_node_at(postXresult, 3) == D);
				}
			}
		}


		// test replace_and_insert_
		AND_GIVEN("an internal-node of [A, B, C]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(2u, A_info, B_info, C_info);
			auto internal_info = atma::_rope_::tree_branch_t<T>{internal_node};

			WHEN("we call replace_and_insert_() at index 2 with node X & nil")
			{
				auto postXresult = atma::_rope_::replace_and_insert_<T>(internal_info, 2, X_info, {});

				THEN("the insert-result contains only one node")
				{
					CHECK(!postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 3);
					CHECK(postXresult.left.info().characters == 3);
					CHECK(postXresult.left.child_count() == 3);
				}

				THEN("the left node contains A, B, X")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
				}
			}

			WHEN("we call replace_and_insert_() at index 2 with node X & Y")
			{
				auto postXresult = atma::_rope_::replace_and_insert_<T>(internal_info, 2, X_info, Y_info);

				THEN("the insert-result contains only one node")
				{
					CHECK(!postXresult.right.has_value());
				}

				THEN("the left node-info is four nodes worth")
				{
					CHECK(postXresult.left.info().bytes == 4);
					CHECK(postXresult.left.info().characters == 4);
					CHECK(postXresult.left.child_count() == 4);
				}

				THEN("the left node contains A, B, X, Y")
				{
					CHECK(lhs_children(postXresult).size() == 4);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
					CHECK(lhs_child_node(postXresult, 3) == Y);
				}

				atma::_rope_::validate_rope_(postXresult.left);
			}
		}
	}
}

SCENARIO("internal text-modifying operations are performed")
{
	using T = atma::rope_test_traits;

	GIVEN("a rope of ['o hey', 'blam\\rdi']")
	{
		auto ohey = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("o hey", 5));
		auto blam_cr = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("blam\rdi", 7));

		atma::_rope_::tree_t<T> ohey_info{ohey};
		atma::_rope_::tree_t<T> blam_info{blam_cr};

		auto root_node = atma::_rope_::make_internal_ptr<T>(2u, ohey_info, blam_info);
		atma::_rope_::tree_t<T> root_info{root_node};

		WHEN("we call atma::_rope_::insert at position 12 (<end>) with anything")
		{
			THEN("the text will be inserted as if we performed a push_back operation")
			{
				auto [left, right, seam] = atma::_rope_::insert<T>(12, root_info, atma::xfer_src("\nzxcv", 5));

				THEN("the result matches our expectations")
				{
					CHECK(left.info().line_breaks == 2);
					CHECK(left.info().characters == 17);
					CHECK(left.info().bytes == 17);
				}
			}
		}

		WHEN("we call atma::_rope_::insert at position 10 ('d') with '\\nzxcv'")
		{
			auto [left, right, seam] = atma::_rope_::insert<T>(10, root_info, atma::xfer_src("\nzxcv", 5));

			THEN("the result matches our expectations")
			{
				CHECK(left.info().line_breaks == 1);
				CHECK(left.info().characters == 17);
				CHECK(left.info().bytes == 17);
			}
		}
	}

	GIVEN("a rope of ['o hey', 'blam\\r']")
	{
		auto ohey = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("o hey", 5));
		auto blam_cr = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("blam\rdi", 5));

		atma::_rope_::tree_t<T> ohey_info{ohey};
		atma::_rope_::tree_t<T> blam_info{blam_cr};

		auto root_node = atma::_rope_::make_internal_ptr<T>(2u, ohey_info, blam_info);
		atma::_rope_::tree_t<T> root_info{root_node};

		WHEN("we call atma::_rope_::insert at position 10 ('d') with '\\nzxcv'")
		{
			auto [left, right, seam] = atma::_rope_::insert<T>(10, root_info, atma::xfer_src("\nzxcv", 5));

			THEN("the result matches our expectations")
			{
				CHECK(left.info().line_breaks == 1);
				CHECK(left.info().characters == 15);
				CHECK(left.info().bytes == 15);
			}
		}
	}
}


SCENARIO("internal rope-building routines are called")
{
	GIVEN("our test passage of text")
	{
		WHEN("we call build_rope_naive")
		{
			auto node_info = atma::_rope_::build_rope_naive<atma::rope_test_traits>(atma::xfer_src(passage, passage_size));

			THEN("the rope root node-info matches the passage")
			{
				CHECK(node_info.info().bytes == passage_size);
				CHECK(node_info.info().characters == passage_size);
				CHECK(node_info.info().dropped_bytes == 0);
				CHECK(node_info.info().dropped_characters == 0);
				CHECK(node_info.info().line_breaks == passage_line_breaks);
			}
		}

		WHEN("we call build_rope_")
		{
			auto node_info = atma::_rope_::build_rope_<atma::rope_test_traits>(atma::xfer_src(passage, passage_size));

			THEN("the node-info matches the passage")
			{
				CHECK(node_info.info().bytes == passage_size);
				CHECK(node_info.info().characters == passage_size);
				CHECK(node_info.info().dropped_bytes == 0);
				CHECK(node_info.info().dropped_characters == 0);
				CHECK(node_info.info().line_breaks == passage_line_breaks);
			}
		}
	}
}



//
// construction
//
SCENARIO("user constructs a rope")
{
	WHEN("a rope is default-constructed")
	{
		test_rope_t rope;

		THEN("it is considered empty")
		{
			CHECK(rope.size() == 0);
			CHECK(rope.size_bytes() == 0);
		}
	}
}


//
//  equality operator
//
SCENARIO("user invokes operator == with arguments (rope, char const*)")
{
	GIVEN("a known passage as a char const*")
	AND_GIVEN("a rope constructed from that passage")
	{
		test_rope_t rope{passage, passage_size};

		WHEN("the two are compared with the equality operator")
		THEN("they evaluate as equal")
		{
			CHECK(rope == passage);
		}
	}

	GIVEN("a default-constructed rope")
	AND_GIVEN("a known passage as a char const*")
	{
		test_rope_t rope;

		WHEN("the two are compared with the equality operator")
		THEN("they evaluate as *not* equal")
		{
			CHECK_FALSE(rope == passage);
		}
	}
}

SCENARIO("user invokes operator == with arguments (rope, rope)")
{
	GIVEN("a rope constructed from a known passage")
	AND_GIVEN("a second rope constructed from the same passage")
	{
		test_rope_t rope1{passage, passage_size};
		test_rope_t rope2{passage, passage_size};

		WHEN("the two ropes are compared with the equality operator")
		THEN("they evaluate as equal")
		{
			CHECK(rope1 == rope2);
		}
	}

	GIVEN("a default-constructed rope")
	AND_GIVEN("a second rope constructed from an arbitrary passage")
	{
		test_rope_t rope1;
		test_rope_t rope2{passage, passage_size};

		WHEN("the two ropes are compared with the equality operator")
		THEN("they evaluate as *not* equal")
		{
			CHECK_FALSE(rope1 == rope2);
		}
	}

	GIVEN("a rope constructed from a known passage")
	AND_GIVEN("a second rope constructed from the same passage, but missing the first word")
	{
		// +4 to skip "good"
		char const* passage2 = passage + 4;
		auto const passage2_size = strlen(passage2);

		test_rope_t rope1{passage, passage_size};
		test_rope_t rope2{passage2, passage2_size};

		WHEN("we insert the missing first word into the second rope, making both ropes lexicographically equivalent")
		{
			rope2.insert(0, "good", 4);

			AND_WHEN("the two ropes are compared with the equality operator")
			THEN("they evaluate as equal")
			{
				CHECK(rope1 == rope2);
			}
		}
	}
}

//
// insertion
//
SCENARIO("user calls rope_t::insert")
{
	GIVEN("a default-constructed rope")
	{
		test_rope_t rope;

		WHEN("rope_t::insert is called with a known passage at index 0")
		{
			rope.insert(0, passage, passage_size);

			THEN("the rope will compare as equal to the passage")
			{
				CHECK(rope == passage);
			}
		}
	}
}


SCENARIO("user erases some of the rope")
{
	GIVEN("a rope constructed from a known passage")
	{
		char const* short_passage = "hello there, you awful monsters you";

		test_rope_t rope{short_passage, strlen(short_passage)};

		WHEN("rope_t::erase is called")
		{
			rope.erase(0, 5);
			
			THEN("the rope will compare as equal to the passage")
			{
				CHECK(rope == short_passage + 5);
			}
		}
	}

	GIVEN("a rope constructed from a known passage")
	{
		char const* short_passage = "hello there, you awful monsters you";

		test_rope_t rope{short_passage, strlen(short_passage)};

		WHEN("rope_t::erase is called")
		{
			rope.erase(6, 11);

			THEN("the rope will compare as equal to the passage")
			{
				CHECK(rope == "hello awful monsters you");
			}
		}
	}

	GIVEN("a rope constructed from a known passage")
	{
		char const* short_passage = "hello there, you awful monsters you";

		test_rope_t rope{short_passage, strlen(short_passage)};

		WHEN("rope_t::erase is called")
		{
			rope.erase(4, 4);

			THEN("the rope will compare as equal to the passage")
			{
				CHECK(rope == "hellere, you awful monsters you");
			}
		}
	}

	GIVEN("a rope constructed from a known passage")
	{
		WHEN("we erase from any index with for certain lengths")
		{
			for (int e = 1; e != 20; ++e)
			{
				CAPTURE(e);
				for (int i = 0; i != passage_size - e; ++i)
				{
					CAPTURE(i);
					test_rope_t rope{passage, passage_size};

					rope.erase(i, e);

					std::string comp_passage;
					{
						comp_passage.append(passage, i);
						comp_passage.append(passage + i + e, passage_size - i - e);
					}

					CHECK(rope == comp_passage.c_str());
				}
			}
		}
	}
}

SCENARIO("user calls rope_t::insert at a valid index")
{
	GIVEN("a rope of traits <4, 9> constructed from a passage")
	{
		WHEN("we insert \"zxcv\" into the rope at any index")
		{
			for (int i = 0; i != passage_size; ++i)
			{
				CAPTURE(i);

				atma::basic_rope_t<atma::rope_basic_traits<4, 9>> rope{passage, passage_size};
				
				rope.insert(i, "zxcv", 4);
				
				std::string comp_passage;
				{
					comp_passage.append(passage, i);
					comp_passage.append("zxcv", 4);
					comp_passage.append(passage + i, passage_size - i);
				}

				CHECK(rope == comp_passage.c_str());
			}
		}
	}

	GIVEN("a rope of traits <8, 9> constructed from a passage")
	{
		WHEN("we insert \"zxcv\" into the rope at any index")
		{
			for (int i = 0; i != passage_size; ++i)
			{
				CAPTURE(i);

				atma::basic_rope_t<atma::rope_basic_traits<8, 9>> rope{passage, passage_size};

				rope.insert(i, "zxcv", 4);

				std::string comp_passage;
				{
					comp_passage.append(passage, i);
					comp_passage.append("zxcv", 4);
					comp_passage.append(passage + i, passage_size - i);
				}

				CHECK(rope == comp_passage.c_str());
			}
		}
	}
}

SCENARIO("user calls rope_t::split at a valid index")
{
	GIVEN("a rope of traits <4, 9> constructed from a passage")
	{
		atma::basic_rope_t<atma::rope_basic_traits<4, 9>> rope{passage, passage_size};

		WHEN("we split the rope at any index")
		{
			for (int i = 0; i != passage_size; ++i)
			{
				auto [left, right] = rope.split(i);

				CHECK(atma::_rope_::validate_rope_(left.root()));
				CHECK(atma::_rope_::validate_rope_(right.root()));
				
				CHECK(left == atma::xfer_src(passage, passage_size).to(i));
				CHECK(right == atma::xfer_src(passage, passage_size).from(i));
			}
		}
	}

	GIVEN("a rope of traits <8, 9> constructed from a passage")
	{
		atma::basic_rope_t<atma::rope_basic_traits<8, 9>> rope{passage, passage_size};

		WHEN("we split the rope at any index")
		{
			for (int i = 0; i != passage_size; ++i)
			{
				auto [left, right] = rope.split(i);

				CHECK(atma::_rope_::validate_rope_(left.root()));
				CHECK(atma::_rope_::validate_rope_(right.root()));
			}
		}
	}
}


