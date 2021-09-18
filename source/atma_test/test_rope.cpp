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

SCENARIO("rope can be constructed" * doctest::skip())
{
	GIVEN("")
	{
		THEN("")
		{
			atma::basic_rope_t<atma::rope_test_traits> rope;
			rope.push_back("ab", 2);
			rope.push_back("cd", 2);
			rope.insert(3, "xy", 2);
			rope.insert(2, "fg", 2);
			rope.insert(2, "ijkl", 4);

			std::cout << rope << std::endl;
		}
	}
}
