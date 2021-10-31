#include <atma/unit_test.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/rope.hpp>
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>

#include <array>
#include <utility>
#include <iostream>
#include <concepts>

import atma.bind;
import atma.rope;
import atma.memory;

using test_rope_t = atma::basic_rope_t<atma::rope_test_traits>;

SCENARIO("atma::rope's internal operations work")
{
	using T = atma::rope_test_traits;

	// just quickly grab the node for an insert/edit-result
	auto lhs_internal_node = [](auto&& x) -> decltype(auto) { return x.lhs.node->known_internal(); };
	auto lhs_children = [&](auto&& x) { return lhs_internal_node(x).children_range(); };
	auto lhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return lhs_children(x)[idx].node; };

	auto rhs_internal_node = [](auto&& x) -> decltype(auto) { return x.maybe_rhs.value().node->known_internal(); };
	auto rhs_children = [&](auto&& x) { return rhs_internal_node(x).children_range(); };
	auto rhs_child_node = [&](auto&& x, size_t idx) -> decltype(auto) { return rhs_children(x)[idx].node; };





	GIVEN("several leaf nodes (\"A\", \"B\", \"C\"...) and corresopnding node-infos")
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
			auto internal_node = atma::_rope_::make_internal_ptr<T>();
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we _rope_::insert_ into our internal-node the node A at index 0")
			{
				auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postAresult.maybe_rhs.has_value());
				}

				THEN("the text-info is the sum information of A")
				{
					CHECK(postAresult.lhs.bytes == 1);
					CHECK(postAresult.lhs.characters == 1);
					CHECK(postAresult.lhs.children == 1);
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
				auto postBresult = atma::_rope_::insert_<T>(postAresult.lhs, 1, B_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postBresult.maybe_rhs.has_value());
				}

				THEN("the text-info is the sum information of A & B")
				{
					CHECK(postBresult.lhs.bytes == 2);
					CHECK(postBresult.lhs.characters == 2);
					CHECK(postBresult.lhs.children == 2);
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
				auto postBresult = atma::_rope_::insert_<T>(postAresult.lhs, 1, B_info);
				auto postCresult = atma::_rope_::insert_<T>(postBresult.lhs, 2, C_info);
				auto postDresult = atma::_rope_::insert_<T>(postCresult.lhs, 3, D_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postDresult.maybe_rhs.has_value());
				}

				THEN("the text-info is the sum information of A & B")
				{
					CHECK(postDresult.lhs.bytes == 4);
					CHECK(postDresult.lhs.characters == 4);
					CHECK(postDresult.lhs.children == 4);
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
				auto postBresult = atma::_rope_::insert_<T>(postAresult.lhs, 0, B_info);
				auto postCresult = atma::_rope_::insert_<T>(postBresult.lhs, 0, C_info);
				auto postDresult = atma::_rope_::insert_<T>(postCresult.lhs, 0, D_info);

				THEN("the insert-result does not contain a rhs")
				{
					CHECK(!postDresult.maybe_rhs.has_value());
				}

				THEN("the text-info is the sum information of A,B,C,D")
				{
					CHECK(postDresult.lhs.bytes == 4);
					CHECK(postDresult.lhs.characters == 4);
					CHECK(postDresult.lhs.children == 4);
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
			auto internal_node = atma::_rope_::make_internal_ptr<T>(A_info, B_info, C_info, D_info);
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we insert X at index 0")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 0, X_info);
			
				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains X, A, B")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == X);
					CHECK(lhs_child_node(postXresult, 1) == A);
					CHECK(lhs_child_node(postXresult, 2) == B);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
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
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains A, X, B")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == X);
					CHECK(lhs_child_node(postXresult, 2) == B);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
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
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains A, B, X")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
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
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 3, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains A, B, C")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == C);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
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
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains A, B, C")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == C);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
				}

				THEN("the rhs node contains D, X")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == D);
					CHECK(rhs_child_node(postXresult, 1) == X);
				}
			}
		}

		AND_GIVEN("a fully-saturated internal-node of [A, B, C, D]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(A_info, B_info, C_info, D_info);
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			AND_WHEN("we insert X at index 0")
			{
				auto postXresult = atma::_rope_::insert_<T>(internal_info, 0, X_info);

				THEN("the insert-result contains two node-infos (a split node)")
				{
					CHECK(postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains X, A, B")
				{
					CHECK(lhs_children(postXresult).size() == 3);
					CHECK(lhs_child_node(postXresult, 0) == X);
					CHECK(lhs_child_node(postXresult, 1) == A);
					CHECK(lhs_child_node(postXresult, 2) == B);
				}

				THEN("the rhs node-info is two nodes worth")
				{
					CHECK(postXresult.maybe_rhs.value().bytes == 2);
					CHECK(postXresult.maybe_rhs.value().characters == 2);
					CHECK(postXresult.maybe_rhs.value().children == 2);
				}

				THEN("the rhs node contains C, D")
				{
					CHECK(rhs_children(postXresult).size() == 2);
					CHECK(rhs_child_node(postXresult, 0) == C);
					CHECK(rhs_child_node(postXresult, 1) == D);
				}
			}
		}

		// test replace_
		AND_GIVEN("a fully-saturated internal-node of [A, B, C, D]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(A_info, B_info, C_info, D_info);
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we call replace_() at index 2 with node X")
			{
				auto postXresult = atma::_rope_::replace_<T>(internal_info, 2, X_info);

				THEN("the insert-result contains only one node")
				{
					CHECK(!postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is four nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 4);
					CHECK(postXresult.lhs.characters == 4);
					CHECK(postXresult.lhs.children == 4);
				}

				THEN("the lhs node contains A, B, X, D")
				{
					CHECK(lhs_children(postXresult).size() == 4);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
					CHECK(lhs_child_node(postXresult, 3) == D);
				}
			}
		}


		// test replace_and_insert_
		AND_GIVEN("a fully-saturated internal-node of [A, B, C]")
		{
			auto internal_node = atma::_rope_::make_internal_ptr<T>(A_info, B_info, C_info);
			auto internal_info = atma::_rope_::node_info_t<T>{internal_node};

			WHEN("we call replace_and_insert_() at index 2 with node X & nil")
			{
				auto postXresult = atma::_rope_::replace_and_insert_<T>(internal_info, 2, X_info, atma::_rope_::maybe_node_info_t<T>());

				THEN("the insert-result contains only one node")
				{
					CHECK(!postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is three nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 3);
					CHECK(postXresult.lhs.characters == 3);
					CHECK(postXresult.lhs.children == 3);
				}

				THEN("the lhs node contains A, B, X")
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
					CHECK(!postXresult.maybe_rhs.has_value());
				}

				THEN("the lhs node-info is four nodes worth")
				{
					CHECK(postXresult.lhs.bytes == 4);
					CHECK(postXresult.lhs.characters == 4);
					CHECK(postXresult.lhs.children == 4);
				}

				THEN("the lhs node contains A, B, X, Y")
				{
					CHECK(lhs_children(postXresult).size() == 4);
					CHECK(lhs_child_node(postXresult, 0) == A);
					CHECK(lhs_child_node(postXresult, 1) == B);
					CHECK(lhs_child_node(postXresult, 2) == X);
					CHECK(lhs_child_node(postXresult, 3) == Y);
				}

				atma::_rope_::validate_rope_(postXresult.lhs);
			}
		}
	}



	GIVEN("a default-constructed rope")
	{
		
		test_rope_t rope;

		auto r = atma::_rope_::build_rope_niave<T>(atma::xfer_src(
			"hello there, this is your captain speaking. "
			"unfortunately we forgot to fill up the plane "
			"before takeoff, sorry for the inconvenience, "
			"but I'm going to need some upstanding people "
			"to get out and push us to the closest petrol "
			"station. for your efforts you'll be rewarded "
			"with a $50 gift-coupon that is redeemable at "
			"any store within the food court."
		));

		//auto postEresult = atma::_rope_::insert_<T>(internal_info, 2, E_info);
		//auto postFresult = atma::_rope_::insert_<T>(internal_node, 0, F_info);
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
