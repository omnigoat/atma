#include <atma/unit_test.hpp>

#include <atma/memory.hpp>


SCENARIO("ranges can be filtered", "[memory/base_memory_t]")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<byte>;

		THEN("sizeof memory_t isn't more than a pointer")
		{
			atma::detail::base_memory_t<byte, empty_allocator> blam;
			static_assert(std::is_empty_v<empty_allocator>, "empty_allocator must actually be empty");

			CHECK(sizeof(blam) == sizeof(byte*));
		}
	}
}
