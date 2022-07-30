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

using test_rope_t = atma::basic_rope_t<atma::rope_test_traits>;

int test()
{
	using T = atma::rope_test_traits;
	
	//atma::_rope_::text_info_t ti;
	//atma::_rope_::node_ptr<T> np;
	//atma::enable_intrusive_ptr_make::template make<atma::_rope_::node_info_t<T>>(ti, np);

	atma::_rope_::node_ptr<T>::make(atma::_rope_::node_type_t::leaf, 1u);

#if 1
	using goodrange = std::span<std::unique_ptr<int>>;
	using badrange = std::span<atma::_rope_::node_ptr<T>>;

	static_assert(std::ranges::range<goodrange>);

	badrange yep;

	(void)std::ranges::begin(yep);
	(void)std::ranges::end(yep);

	static_assert(std::ranges::range<badrange>);
#endif

	return 4;
}


SCENARIO("atma::rope's internal operations work")
{
	using T = atma::rope_test_traits;

	// just quickly grab the node for an insert/edit-result
	auto internal_node_of = [](auto&& x) -> decltype(auto) { return x.node->as_branch(); };
	auto children_of = [&](auto&& x) -> decltype(auto) { return internal_node_of(x).children(); };
	auto child_node_at = [&](auto&& x, size_t idx) -> decltype(auto) { return children_of(x)[idx].node; };

	auto lhs_internal_node = [&](auto&& x) -> decltype(auto) { return internal_node_of(x.left); };
	auto lhs_children = [&](auto&& x) { return children_of(x.left); };
	auto lhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return child_node_at(x.left, idx); };

	auto rhs_internal_node = [&](auto&& x) -> decltype(auto) { return internal_node_of(x.right.value()); };
	auto rhs_children = [&](auto&& x) { return children_of(x.right.value()); };
	auto rhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return child_node_at(x.right.value(), idx); };

	
	//using Range = decltype(internal_node.children());
	//using F = decltype(atma::bind_from<1>(&check_node<RT>, RT::minimum_branches));
	//static_assert(!std::is_reference_v<Range>);
#if 0
	using goodrange = std::span<std::unique_ptr<int>>;
	using badrange = std::span<atma::_rope_::node_ptr<T>>;

	static_assert(std::ranges::range<goodrange>);

	badrange yep;

	(void)std::ranges::begin(yep);
	(void)std::ranges::end(yep);

	static_assert(std::ranges::range<badrange>);
#endif



#if 0
	GIVEN("several leaf nodes (\"A\", \"B\", \"C\"...) and corresponding node-infos")
	{
		auto A = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("A", 1));
		auto B = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("B", 1));
		auto C = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("C", 1));
		auto D = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("D", 1));
		auto X = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("X", 1));
		auto Y = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("Y", 1));

		atma::_rope_::node_info_t<T> A_info{A};
		atma::_rope_::node_info_t<T> B_info{B};
		atma::_rope_::node_info_t<T> C_info{C};
		atma::_rope_::node_info_t<T> D_info{D};
		atma::_rope_::node_info_t<T> X_info{X};
		atma::_rope_::node_info_t<T> Y_info{Y};

		// test insert where we have space
		AND_GIVEN("a default-created internal-node")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(1u);
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we _rope_::insert_ into our internal-node the node A at index 0")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postAresult.right.has_value());
				}

				THEN("the text-info is the sum information of A")
				{
					CHECK(postAresult.left.bytes == 1);
					CHECK(postAresult.left.characters == 1);
					CHECK(postAresult.left.children == 1);
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
					CHECK(postBresult.left.bytes == 2);
					CHECK(postBresult.left.characters == 2);
					CHECK(postBresult.left.children == 2);
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
					CHECK(postDresult.left.bytes == 4);
					CHECK(postDresult.left.characters == 4);
					CHECK(postDresult.left.children == 4);
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
					CHECK(postDresult.left.bytes == 4);
					CHECK(postDresult.left.characters == 4);
					CHECK(postDresult.left.children == 4);
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
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we insert X at index 0")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 0, X_info);
			
				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.right.value().bytes == 2);
					CHECK(postXresult.right.value().characters == 2);
					CHECK(postXresult.right.value().children == 2);
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
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.right.value().bytes == 2);
					CHECK(postXresult.right.value().characters == 2);
					CHECK(postXresult.right.value().children == 2);
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
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.right.value().bytes == 2);
					CHECK(postXresult.right.value().characters == 2);
					CHECK(postXresult.right.value().children == 2);
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
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.right.value().bytes == 2);
					CHECK(postXresult.right.value().characters == 2);
					CHECK(postXresult.right.value().children == 2);
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
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.right.value().bytes == 2);
					CHECK(postXresult.right.value().characters == 2);
					CHECK(postXresult.right.value().children == 2);
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
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we call replace_() at index 2 with node X")
			{
				auto postXresult = atma::_rope_::replace_<T>(internal_info, 2, X_info);

				THEN("the left node-info is four nodes worth")
				{
					CHECK(postXresult.bytes == 4);
					CHECK(postXresult.characters == 4);
					CHECK(postXresult.children == 4);
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
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we call replace_and_insert_() at index 2 with node X & nil")
			{
				auto postXresult = atma::_rope_::replace_and_insert_<T>(internal_info, 2, X_info, atma::_rope_::maybe_node_info_t<T>());

				THEN("the insert-result contains only one node")
				{
					CHECK(!postXresult.right.has_value());
				}

				THEN("the left node-info is three nodes worth")
				{
					CHECK(postXresult.left.bytes == 3);
					CHECK(postXresult.left.characters == 3);
					CHECK(postXresult.left.children == 3);
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
					CHECK(postXresult.left.bytes == 4);
					CHECK(postXresult.left.characters == 4);
					CHECK(postXresult.left.children == 4);
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
#endif
}

SCENARIO("rope can be build from text")
{
	GIVEN("our test passage of text")
	{
		char const* passage = 
			"hello there, this is your captain speaking.  \n"
			"unfortunately we forgot to fill up the plane \n"
			"before takeoff. sorry for the inconvenience, \n"
			"but I'm going to need some upstanding people \n"
			"to get out and push us to the closest petrol \n"
			"station. for your efforts you'll be rewarded \n"
			"with a $50 gift-coupon that is redeemable at \n"
			"any store within the food court.";

		auto const passage_size = strlen(passage);

		WHEN("we call build_rope_naive")
		{
			auto node_info = atma::_rope_::build_rope_naive<atma::rope_test_traits>(atma::xfer_src(passage, passage_size));

			THEN("the rope root node-info matches the passage")
			{
				CHECK(node_info.bytes == passage_size);
				CHECK(node_info.characters == passage_size);
				CHECK(node_info.dropped_bytes == 0);
				CHECK(node_info.dropped_characters == 0);
				CHECK(node_info.line_breaks == 7);
			}
		}

		WHEN("we call build_rope_")
		{
			auto rope = atma::_rope_::build_rope_<atma::rope_test_traits>(atma::xfer_src(passage, passage_size));

			THEN("the rope root node-info matches the passage")
			{
				auto const& root = rope.root();

				CHECK(root.bytes == passage_size);
				CHECK(root.characters == passage_size);
				CHECK(root.dropped_bytes == 0);
				CHECK(root.dropped_characters == 0);
				CHECK(root.line_breaks == 7);
			}
		}
	}

	
}


#if 0
SCENARIO("atma::rope equality operators function")
{
	GIVEN("a known passage as a char const*")
	{
		char const* passage =
			"hello there, this is your captain speaking.  \n"
			"unfortunately we forgot to fill up the plane \n"
			"before takeoff. sorry for the inconvenience, \n"
			"but I'm going to need some upstanding people \n"
			"to get out and push us to the closest petrol \n"
			"station. for your efforts you'll be rewarded \n"
			"with a $50 gift-coupon that is redeemable at \n"
			"any store within the food court.";
		auto const passage_size = strlen(passage);

		AND_GIVEN("a rope constructed from that passage")
		{
			auto rope = atma::_rope_::build_rope_<atma::rope_test_traits>(atma::xfer_src(passage, passage_size));

			THEN("the rope equates to the passage")
			{
				CHECK(rope == passage);
			}
		
			AND_GIVEN("another rope of the same passage but missing the first word")
			{
				char const* passage2 =
					" there, this is your captain speaking.  \n"
					"unfortunately we forgot to fill up the plane \n"
					"before takeoff. sorry for the inconvenience, \n"
					"but I'm going to need some upstanding people \n"
					"to get out and push us to the closest petrol \n"
					"station. for your efforts you'll be rewarded \n"
					"with a $50 gift-coupon that is redeemable at \n"
					"any store within the food court.";

				auto const passage2_size = strlen(passage2);
				auto rope2 = atma::_rope_::build_rope_<atma::rope_test_traits>(atma::xfer_src(passage2, passage2_size));

				WHEN("we insert the first word into the second passage, making both passages the same")
				{
					rope2.insert(0, "hello", 5);

					THEN("the two ropes equate to each other")
					{
						CHECK(rope == rope2);
					}
				}
			}
		}
	}
}




#if 0
SCENARIO("inserting")
{
	GIVEN("a default-constructed rope")
	{
		char const* passage =
			"hello there, this is your captain speaking.  \n"
			"unfortunately we forgot to fill up the plane \n"
			"before takeoff. sorry for the inconvenience, \n"
			"but I'm going to need some upstanding people \n"
			"to get out and push us to the closest petrol \n"
			"station. for your efforts you'll be rewarded \n"
			"with a $50 gift-coupon that is redeemable at \n"
			"any store within the food court.";
		auto const passage_size = strlen(passage);

		auto const* insert = 
			"haha just kidding. what I actually need is \n"
			"for everyone to get under the plane and blow \n"
			"upwards to keep us flying. ";
		auto const insert_size = strlen(insert);

		using rope_test_traits_2 = atma::rope_basic_traits<8, 9>;

		auto rope = atma::_rope_::build_rope_<rope_test_traits_2>(atma::xfer_src(passage, passage_size));

		WHEN("we split the rope")
		{
			rope.insert(239, insert, insert_size);

			//THEN("something something")
			//{
			//	std::cout << rope << std::endl;
			//}
		}
	}
}
#endif




SCENARIO("splitting")
{
	GIVEN("a standard passage")
	{
		char const* passage =
			"hello there, this is your captain speaking.  \n"
			"unfortunately we forgot to fill up the plane \n"
			"before takeoff. sorry for the inconvenience, \n"
			"but I'm going to need some upstanding people \n"
			"to get out and push us to the closest petrol \n"
			"station. for your efforts you'll be rewarded \n"
			"with a $50 gift-coupon that is redeemable at \n"
			"any store within the food court.";

		auto const passage_size = strlen(passage);
	
		AND_GIVEN("a default-constructed rope of <4, 9>")
		{
			auto rope = atma::_rope_::build_rope_<atma::rope_basic_traits<4, 9>>(atma::xfer_src(passage, passage_size));

			WHEN("we split the rope")
			{
				for (int i = 0; i != passage_size; ++i)
				{
					auto [left, right] = rope.split(i);

					CHECK(atma::_rope_::validate_rope_(left.root()));
					CHECK(atma::_rope_::validate_rope_(right.root()));
				}
			}
		}

		AND_GIVEN("a default-constructed rope of <8, 9>")
		{
			auto rope = atma::_rope_::build_rope_<atma::rope_basic_traits<8, 9>>(atma::xfer_src(passage, passage_size));

			WHEN("we split the rope")
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
}



SCENARIO("seams" * doctest::skip())
{
	GIVEN("a default-constructed rope")
	{
		test_rope_t rope;

		THEN("")
		{
			rope.push_back("abdefg", 6);
			rope.push_back("\n123456", 7);
			rope.insert(6, "\r", 1);

			std::cout << rope << std::endl;
		}
	}
}



SCENARIO("rope can be inserted" * doctest::skip())
{
	GIVEN("a default-constructed rope")
	{
		test_rope_t rope;

		THEN("")
		{
			rope.push_back("ab", 2);
			rope.push_back("cd", 2);
			rope.insert(3, "xy", 2);
			rope.insert(2, "fg", 2);
			rope.insert(2, "ijkl", 4);

			std::cout << rope << std::endl;
		}
	}
}
#endif