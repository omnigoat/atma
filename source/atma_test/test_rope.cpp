#include <atma/unit_test.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/rope.hpp>
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atma/ranges/core.hpp>
#include <atma/algorithm.hpp>
#include <atma/utf/utf8_string.hpp>
#include <atma/bind.hpp>

#include <array>
#include <utility>
#include <iostream>

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

	GIVEN("a default-created internal-node, and corresponding node-info")
	AND_GIVEN("several leaf nodes (\"A\", \"B\", \"C\"...) and corresopnding node-infos")
	{
		auto internal_node = atma::_rope_::make_internal_ptr<T>();
		atma::_rope_::node_info_t<T> internal_info{internal_node};

		auto A = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("A", 1));
		atma::_rope_::node_info_t<T> A_info{A};

		auto B = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("B", 1));
		atma::_rope_::node_info_t<T> B_info{B};

		auto C = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("C", 1));
		atma::_rope_::node_info_t<T> C_info{C};

		auto D = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("D", 1));
		atma::_rope_::node_info_t<T> D_info{D};

		auto E = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("E", 1));
		atma::_rope_::node_info_t<T> E_info{E};

		auto F = atma::_rope_::make_leaf_ptr<T>(atma::xfer_src("F", 1));
		atma::_rope_::node_info_t<T> F_info{F};

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

		THEN("into internal-node we can insert_ nodes A, B, C, D at indices 0, 1, 2, 3 to saturate the node")
		{
			auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);
			auto postBresult = atma::_rope_::insert_<T>(postAresult.lhs, 1, B_info);
			auto postCresult = atma::_rope_::insert_<T>(postBresult.lhs, 2, C_info);
			auto postDresult = atma::_rope_::insert_<T>(postCresult.lhs, 3, D_info);

			CHECK(postDresult.lhs.bytes == 4);
			CHECK(postDresult.lhs.characters == 4);
			CHECK(postDresult.lhs.children == 4);
			CHECK(lhs_child_node(postDresult, 0) == A);
			CHECK(lhs_child_node(postDresult, 1) == B);
			CHECK(lhs_child_node(postDresult, 2) == C);
			CHECK(lhs_child_node(postDresult, 3) == D);
		}

		THEN("into internal-node we can insert_ nodes A, B, C, D at index 0 to saturate the node")
		{
			auto postAresult = atma::_rope_::insert_<T>(internal_info, 0, A_info);
			auto postBresult = atma::_rope_::insert_<T>(postAresult.lhs, 1, B_info);
			auto postCresult = atma::_rope_::insert_<T>(postBresult.lhs, 2, C_info);
			auto postDresult = atma::_rope_::insert_<T>(postCresult.lhs, 3, D_info);

			CHECK(postDresult.lhs.bytes == 4);
			CHECK(postDresult.lhs.characters == 4);
			CHECK(postDresult.lhs.children == 4);
			CHECK(lhs_child_node(postDresult, 0) == A);
			CHECK(lhs_child_node(postDresult, 1) == B);
			CHECK(lhs_child_node(postDresult, 2) == C);
			CHECK(lhs_child_node(postDresult, 3) == D);
		}
	}


	GIVEN("a default-constructed rope")
	{
		
		test_rope_t rope;

		
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
