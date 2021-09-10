#include <atma/unit_test.hpp>

#include <atma/utf/utf8_string.hpp>


SCENARIO_TEMPLATE("utf8_span_t is used by user", span_type, atma::utf8_span_t, atma::utf8_const_span_t)
{
	WHEN("utf8_span is default-constructed")
	{
		span_type span;

		THEN("it points to nullptr")
		AND_THEN("its size is zero")
		AND_THEN(".empty() evaluates to true")
		{
			CHECK(span.data() == nullptr);
			CHECK(span.size() == 0);
			CHECK(span.empty());
		}
	}

	WHEN("utf8_span is copy-constructed")
	{
		atma::utf8_string_t test{"hello good sir"};

		GIVEN("an originating span constructed around the range 'hello good sir'")
		AND_GIVEN("a span copy-constructed from our originating span")
		{
			span_type span1{test};
			span_type span2{span1};

			THEN("the copy-constructed span equates its originating span")
			{
				CHECK(span2.data() == span1.data());
				CHECK(span2.size() == span1.size());
			}
		}
	}

	WHEN("utf8_span is direct-constructed from a range")
	{
		atma::utf8_string_t test{"hello good sir"};

		static_assert(std::input_or_output_iterator<atma::utf8_string_t::iterator>);
		static_assert(std::ranges::range<atma::utf8_string_t&>, "not a range");
		static_assert(std::ranges::forward_range<atma::utf8_string_t&>, "not a range");
		
		(void)std::ranges::begin(test);
		(void)std::ranges::end(test);

		GIVEN("a utf8_string_t of 'hello good sir'")
		AND_GIVEN("a utf8_span directly-constructed from said string")
		{
			span_type span{test};

			THEN("the directly constructed span covers the length of the string")
			{
				CHECK(span.data() == test.data());
				CHECK(span.size_bytes() == test.size_bytes());
			}
		}
	}
}

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
			CHECK(s1.size_bytes() == 6);
			CHECK(s1 == "dragon");

			CHECK(s2.size_bytes() == 6);
			CHECK(s2 == "dragon");

			CHECK(s2.size_bytes() == 6);
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
			CHECK(s1.size_bytes() == 17);
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

#if 0
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
			auto i = atma::find_first_of(b, s.end(), "ao");
			CHECK(i != s.end());
			CHECK(std::distance(s.begin(), i) == 4);
			CHECK(*i == 'o');
		}
#endif

		THEN("we can find the substr \"the\"")
		{
			//auto i = atma::find_substr(s, "the");
		}
	}
}
