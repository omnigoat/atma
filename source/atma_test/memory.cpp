#include <atma/unit_test.hpp>

#include <atma/memory.hpp>


SCENARIO("base_memory_t can EBO properly", "[memory/base_memory_t]")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<byte>;
		static_assert(std::is_empty_v<empty_allocator>, "empty_allocator must actually be empty");

		THEN("sizeof memory_t isn't more than a pointer")
		{
			atma::detail::base_memory_t<byte, empty_allocator> memory;
			CHECK(sizeof(memory) == sizeof(byte*));
		}
	}
}


SCENARIO("simple_memory_t behaves nicely", "[memory/simple_memory_t]")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<byte>;
		using memory_t = atma::simple_memory_t<byte, empty_allocator>;

		THEN("simple_memory_t can be default-constructed")
		{
			auto memory = memory_t();
		}

		THEN("simple_memory_t can constructed from a pointer & allocator")
		{
			byte* ptr = nullptr;
			auto memory = memory_t(ptr);
		}
	}
}


SCENARIO("basic_memory_t behaves nicely", "[memory/basic_memory_t]")
{
	GIVEN("an empty allocator")
	{
		using empty_allocator = atma::aligned_allocator_t<byte>;
		using memory_t = atma::basic_memory_t<byte, empty_allocator>;

		THEN("basic_memory_t can be default-constructed")
		{
			auto memory = memory_t();
		}

		THEN("basic_memory_t can constructed from a pointer")
		{
			byte* ptr = nullptr;
			auto memory = memory_t(ptr);
		}

		THEN("basic_memory_t can constructed from a pointer")
		{
			byte* ptr = nullptr;
			auto memory = memory_t(ptr);
		}
	}
}