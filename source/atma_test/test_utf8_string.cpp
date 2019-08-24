#include <atma/unit_test.hpp>

#include <atma/utf/utf8_string.hpp>


SCENARIO_OF("utf8_string_t", "utf8-strings can be constructed")
{
	GIVEN("a default-constructed string")
	{
		atma::utf8_string_t s;

		THEN("it is empty")
		{
			CHECK(s.empty());
			CHECK(s.begin() == s.end());
			CHECK(s.raw_begin() == s.raw_end());
		}
	}

	GIVEN("strings constructed with \"dragon\"")
	{
		char const* t = "dragons dancing";

		atma::utf8_string_t s1{"dragon"};
		atma::utf8_string_t s2{"dragons", 6};
		atma::utf8_string_t s3{t, t + 6};

		THEN("they compare to \"dragon\"")
		{
			CHECK(s1.raw_size() == 6);
			CHECK(s1 == "dragon");

			CHECK(s2.raw_size() == 6);
			CHECK(s2 == "dragon");

			CHECK(s2.raw_size() == 6);
			CHECK(s3 == "dragon");
		}

		THEN("they compare to each other")
		{
			CHECK(s1 == s2);
			CHECK(s2 == s3);
			CHECK(s1 == s3);
		}
	}

	GIVEN("strings constructed with hard chars {ô, 擿, 銌, 뮨}")
	{
		atma::utf8_string_t s1{"ô, 擿, 銌, 뮨"};
		
		// this is s1 when win32 consoles fail to display correct unicode
		atma::utf8_string_t s2{"├┤, µô┐, Úèî, Ù«¿"};

		THEN("they don't equate to the common erroneous encoding {├┤, µô┐, Úèî, Ù«¿}")
		{
			CHECK(s1.raw_size() == 17);
			CHECK(s1 != s2);
		}
	}

	GIVEN("a copy-constructed string")
	{
		atma::utf8_string_t s1{"dragon"};
		auto s2 = s1;

		THEN("it equates to the original")
		{
			CHECK(s2 == s1);
		}
	}

	GIVEN("the string \"dragons 擿 in the sky\"")
	{
		atma::utf8_string_t s{"dragons 擿 in the sky"};

		THEN("we can find-first-of \"ao\" at index 2")
		{
			auto i = atma::find_first_of(s, "ao");
			CHECK(i != s.end());
			CHECK(std::distance(s.begin(), i) == 2);
			CHECK(*i == 'a');
		}

		THEN("we can find-first-of \"ao\" at index 4 if starting late enough")
		{
			auto b = ++++++++s.begin(); // 'g'
			auto i = atma::find_first_of(s, b, "ao");
			CHECK(i != s.end());
			CHECK(std::distance(s.begin(), i) == 4);
			CHECK(*i == 'o');
		}

		THEN("we can find the substr \"the\"")
		{
			//auto i = atma::find_substr(s, "the");
		}
	}
}
