#include <atma/unit_test.hpp>

#include <atma/utf/utf8_string.hpp>

SCENARIO("utf8_strings can be constructed", "[utf8_string]")
{
	GIVEN("a default-constructed utf8-string")
	{
		atma::utf8_string_t s;

		CHECK(s.empty());
		CHECK(s.raw_size() == 0);
	}
}