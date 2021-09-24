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


SCENARIO("rope can be constructed" * doctest::skip())
{
	using T = atma::rope_test_traits;
	test_rope_t rope;

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

	auto postAresult = atma::_rope_::insert_<T>(internal_node, 0, A_info);
	auto postBresult = atma::_rope_::insert_<T>(internal_node, 1, B_info);
	auto postCresult = atma::_rope_::insert_<T>(internal_node, 2, C_info);
	auto postDresult = atma::_rope_::insert_<T>(internal_node, 3, D_info);
	auto postEresult = atma::_rope_::insert_<T>(internal_node, 2, E_info);
	//auto postFresult = atma::_rope_::insert_<T>(internal_node, 0, F_info);
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
